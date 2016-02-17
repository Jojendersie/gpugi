#include "hierarchyimportance.hpp"
#include "renderersystem.hpp"
#include "debugrenderer/raytracemeshinfo.hpp"
#include "debugrenderer/hierarchyvisualization.hpp"

#include "scene/scene.hpp"

#include <glhelper/texture2d.hpp>
#include <glhelper/buffer.hpp>
#include <glhelper/texturebufferview.hpp>
#include <glhelper/utils/flagoperators.hpp>
#include <glhelper/texturecubemap.hpp>

#include <fstream>

using namespace std;

const ei::UVec2 HierarchyImportance::m_localSizePathtracer = ei::UVec2(8, 8);

HierarchyImportance::HierarchyImportance(RendererSystem& _rendererSystem) :
	Renderer(_rendererSystem),
	m_hierarchyImpAcquisitionShader("hierarchyImpAcquisition"),
	m_hierarchyImpPropagationInitShader("hierarchyImpPropagation_Init"),
	m_hierarchyImpPropagationNodeShader("hierarchyImpPropagation_Node"),
	m_hierarchyPathTracer("hierarchyPathTracer")
{
	m_rendererSystem.SetNumInitialLightSamples(128);
}

void HierarchyImportance::SetScene(shared_ptr<Scene> _scene)
{
	RecompileShaders(_scene->GetBvhTypeDefineString());
	// Contains an importance value (float) for each node (first) and each triangle (after node values)
	m_hierarchyImportance = make_shared<gl::Buffer>(sizeof(float) * (_scene->GetNumLeafTriangles() + _scene->GetNumInnerNodes()), gl::Buffer::IMMUTABLE);
//	m_hierarchyImportance->ClearToZero();
	//m_hierarchyImportance->BindShaderStorageBuffer((uint)Binding::HIERARCHY_IMPORTANCE);
	m_hierachyImportanceView = make_unique<gl::TextureBufferView>(m_hierarchyImportance, gl::TextureBufferFormat::R32F);
	m_hierachyImportanceView->BindBuffer((uint)Binding::HIERARCHY_IMPORTANCE);
	m_subtreeImportance = make_shared<gl::Buffer>(sizeof(float) * _scene->GetNumInnerNodes(), gl::Buffer::IMMUTABLE);
	m_subtreeImportance->ClearToZero();
	m_subtreeImportance->BindShaderStorageBuffer((uint)Binding::SUBTREE_IMPORTANCE);

	gl::MappedUBOView mapView(m_hierarchyImportanceUBOInfo, m_hierarchyImportanceUBO->Map(gl::Buffer::MapType::WRITE, gl::Buffer::MapWriteFlag::NONE));
	mapView["NumInnerNodes"].Set(static_cast<int32_t>(_scene->GetNumInnerNodes()));
	mapView["NumTriangles"].Set(static_cast<int32_t>(_scene->GetNumLeafTriangles()));
	m_hierarchyImportanceUBO->Unmap();

	// Parent pointer for hierarchy propagation.
	m_sceneParentPointer = make_unique<gl::TextureBufferView>(_scene->GetParentBuffer(), gl::TextureBufferFormat::R32I);
	m_sceneParentPointer->BindBuffer((uint)Binding::PARENT_POINTER);

	if(!_scene->GetSGGXBuffer())
		LOG_ERROR("Requires SGGX NDFs for hierarchy to apply importance based rendering!");
	else {
		m_sggxBufferView = make_unique<gl::TextureBufferView>(_scene->GetSGGXBuffer(), gl::TextureBufferFormat::RG16);
		m_sggxBufferView->BindBuffer((uint)Binding::SGGX_NDF);
	}

	m_hierarchyMaterialBuffer = make_shared<gl::Buffer>(2 * 4 * 6 * _scene->GetNumInnerNodes(), gl::Buffer::IMMUTABLE);
	m_hierarchyMaterialBufferView = make_unique<gl::TextureBufferView>(m_hierarchyMaterialBuffer, gl::TextureBufferFormat::RGBA16F);
	m_hierarchyMaterialBufferView->BindBuffer((uint)Binding::HIERARCHY_MATERIAL);
	ComputeHierarchyMaterials(_scene);
}

void HierarchyImportance::SetScreenSize(const gl::Texture2D& _newBackbuffer)
{
//	if (m_hierarchyImportance)
//		m_hierarchyImportance->ClearToZero();

	_newBackbuffer.BindImage(0, gl::Texture::ImageAccess::READ_WRITE);
}

void HierarchyImportance::SetEnvironmentMap(std::shared_ptr<gl::TextureCubemap> _envMap)
{
	_envMap->Bind(12);
}

void HierarchyImportance::Draw()
{
	// Reset importance on camera movement...
	if(m_rendererSystem.GetIterationCount() == 0)
	{
		m_hierarchyImportance->BindShaderStorageBuffer((uint)Binding::HIERARCHY_IMPORTANCE);
		m_hierarchyImportance->ClearToZero();
		m_subtreeImportance->ClearToZero();

		m_numImportanceIterations = 10;
		for(int i = 1; i <= m_numImportanceIterations; ++i) // Until convergence (TODO measure that)
		{
			gl::MappedUBOView mapView(m_hierarchyImportanceUBOInfo, m_hierarchyImportanceUBO->Map(gl::Buffer::MapType::WRITE, gl::Buffer::MapWriteFlag::NONE));
			mapView["NumImportanceIterations"].Set(static_cast<int32_t>(i));
			m_hierarchyImportanceUBO->Unmap();

			// Compute triangle importance
			m_hierarchyImpAcquisitionShader.Activate();
			GL_CALL(glDispatchCompute, m_rendererSystem.GetBackbuffer().GetWidth() / m_localSizePathtracer.x, m_rendererSystem.GetBackbuffer().GetHeight() / m_localSizePathtracer.y, 1);
			GL_CALL(glMemoryBarrier, GL_SHADER_STORAGE_BARRIER_BIT);
		}
		// Propagate importance through hierarchy
		UpdateHierarchyNodeImportance();
		gl::Buffer::BindShaderStorageBuffer(0, (uint)Binding::HIERARCHY_IMPORTANCE, 0, 0);
	}

	// Render
	m_hierachyImportanceView->BindBuffer((uint)Binding::HIERARCHY_IMPORTANCE);
	//GL_CALL(glMemoryBarrier, GL_ALL_BARRIER_BITS);
	//GL_CALL(glFinish);
	m_hierarchyPathTracer.Activate();
	GL_CALL(glDispatchCompute, m_rendererSystem.GetBackbuffer().GetWidth() / m_localSizePathtracer.x, m_rendererSystem.GetBackbuffer().GetHeight() / m_localSizePathtracer.y, 1);
}

void HierarchyImportance::UpdateHierarchyNodeImportance()
{
	// First pull importance from the triangles into the nodes.
	// Then pull importance from child nodes, tree layer by tree layer until convergence
	//
	// Why not in a single shader?
	//  Idea would be to perform "pull tries" that finish only if there is importance to pull and busy loop otherwise.
	//  This is not possible with GPUs since a running shader invocation might be waiting for another invocation that was not yet triggered!

	GLuint numBlocks = (m_rendererSystem.GetScene()->GetNumInnerNodes() + 63) / 64;

	m_hierarchyImpPropagationInitShader.Activate();
	GL_CALL(glDispatchCompute, numBlocks, 1, 1);
	GL_CALL(glMemoryBarrier, GL_SHADER_STORAGE_BARRIER_BIT);


	gl::Buffer doneBuffer(4, gl::Buffer::MAP_READ | gl::Buffer::MAP_WRITE);
	doneBuffer.BindShaderStorageBuffer(1);
	m_hierarchyImpPropagationNodeShader.Activate();

	bool changedAnything = false;
	do
	{
		*static_cast<int*>(doneBuffer.Map(gl::Buffer::MapType::WRITE, gl::Buffer::MapWriteFlag::NONE)) = 0;
		doneBuffer.Unmap();

		GL_CALL(glDispatchCompute, numBlocks, 1, 1);
		GL_CALL(glMemoryBarrier, GL_SHADER_STORAGE_BARRIER_BIT);

		changedAnything = *static_cast<int*>(doneBuffer.Map(gl::Buffer::MapType::READ, gl::Buffer::MapWriteFlag::NONE)) != 0;
		doneBuffer.Unmap();
	} while (changedAnything);
}

void HierarchyImportance::ComputeHierarchyMaterials(shared_ptr<Scene> _scene)
{
	string additionalDefines = _scene->GetBvhTypeDefineString();
	// To average the material per leaf node we need to sample the textures on
	// all triangles uniformly. Best to do that in a shader...
	unique_ptr<gl::ShaderObject> sampleLeaves = make_unique<gl::ShaderObject>("sampleLeafMaterials");
	sampleLeaves->AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/hierarchy/sampleleafmaterials.comp", additionalDefines);
	sampleLeaves->CreateProgram();
	sampleLeaves->Activate();
	m_hierarchyMaterialBuffer->BindShaderStorageBuffer(1);
	GLuint numBlocks = (_scene->GetNumInnerNodes() + 63) / 64;
	GL_CALL(glDispatchCompute, numBlocks, 1, 1);
	GL_CALL(glMemoryBarrier, GL_SHADER_STORAGE_BARRIER_BIT);

	// Pull materials up in the hierarchy
	unique_ptr<gl::ShaderObject> pullMaterials = make_unique<gl::ShaderObject>("pullMaterials");
	pullMaterials->AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/hierarchy/pullmaterials.comp", additionalDefines);
	pullMaterials->CreateProgram();
	pullMaterials->Activate();
	for(uint i=0; i < _scene->GetNumTreeLevels(); ++i)
	{
		GL_CALL(glDispatchCompute, numBlocks, 1, 1);
		GL_CALL(glMemoryBarrier, GL_SHADER_STORAGE_BARRIER_BIT);
	}
}

void HierarchyImportance::RecompileShaders(const string& _additionalDefines)
{
	string additionalDefines = _additionalDefines;
#ifdef SHOW_SPECIFIC_PATHLENGTH
	additionalDefines += "#define SHOW_SPECIFIC_PATHLENGTH " + std::to_string(SHOW_SPECIFIC_PATHLENGTH) + "\n";
#endif

	m_hierarchyImpAcquisitionShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/hierarchy/acquisition2.comp", additionalDefines);
	m_hierarchyImpAcquisitionShader.CreateProgram();
	m_hierarchyImpPropagationInitShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/hierarchy/hierarchypropagation_init.comp", additionalDefines);
	m_hierarchyImpPropagationInitShader.CreateProgram();
	m_hierarchyImpPropagationNodeShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/hierarchy/hierarchypropagation_nodes.comp", additionalDefines);
	m_hierarchyImpPropagationNodeShader.CreateProgram();
	m_hierarchyPathTracer.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/hierarchy/hierarchypathtracer.comp", additionalDefines);
	m_hierarchyPathTracer.CreateProgram();

	m_hierarchyImportanceUBOInfo = m_hierarchyImpAcquisitionShader.GetUniformBufferInfo()["HierarchyImportanceUBO"];
	m_hierarchyImportanceUBO = make_unique<gl::Buffer>(m_hierarchyImportanceUBOInfo.bufferDataSizeByte, gl::Buffer::MAP_WRITE);
	m_hierarchyImportanceUBO->BindUniformBuffer(4);
}

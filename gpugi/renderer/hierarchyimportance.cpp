#include "hierarchyimportance.hpp"
#include "renderersystem.hpp"
#include "debugrenderer/raytracemeshinfo.hpp"
#include "debugrenderer/hierarchyvisualization.hpp"

#include "../scene/scene.hpp"

#include <glhelper/texture2d.hpp>
#include <glhelper/buffer.hpp>
#include <glhelper/texturebufferview.hpp>
#include <glhelper/utils/flagoperators.hpp>

#include <fstream>

const ei::UVec2 HierarchyImportance::m_localSizePathtracer = ei::UVec2(8, 8);

HierarchyImportance::HierarchyImportance(RendererSystem& _rendererSystem) :
	Renderer(_rendererSystem),
	m_hierarchyImpAcquisitionShader("hierarchyImpAcquisition"),
	m_hierarchyImpPropagationInitShader("hierarchyImpPropagation_Init"),
	m_hierarchyImpPropagationNodeShader("hierarchyImpPropagation_Node"),
	m_hierarchyPathTracer("hierarchyPathTracer")
{
	std::string additionalDefines;
#ifdef SHOW_SPECIFIC_PATHLENGTH
	additionalDefines += "#define SHOW_SPECIFIC_PATHLENGTH " + std::to_string(SHOW_SPECIFIC_PATHLENGTH) + "\n";
#endif

	m_hierarchyImpAcquisitionShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/hierarchy/acquisition.comp", additionalDefines);
	m_hierarchyImpAcquisitionShader.CreateProgram();
	m_hierarchyImpPropagationInitShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/hierarchy/hierarchypropagation_init.comp");
	m_hierarchyImpPropagationInitShader.CreateProgram();
	m_hierarchyImpPropagationNodeShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/hierarchy/hierarchypropagation_nodes.comp");
	m_hierarchyImpPropagationNodeShader.CreateProgram();
	m_hierarchyPathTracer.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/hierarchy/hierarchypathtracer.comp", additionalDefines + "#define TRACERAY_IMPORTANCE_BREAK");
	m_hierarchyPathTracer.CreateProgram();

	m_hierarchyImportanceUBOInfo = m_hierarchyImpAcquisitionShader.GetUniformBufferInfo()["HierarchyImportanceUBO"];
	m_hierarchyImportanceUBO = std::make_unique<gl::Buffer>(m_hierarchyImportanceUBOInfo.bufferDataSizeByte, gl::Buffer::MAP_WRITE);
	m_hierarchyImportanceUBO->BindUniformBuffer(4);

	m_rendererSystem.SetNumInitialLightSamples(128);
}

void HierarchyImportance::SetScene(std::shared_ptr<Scene> _scene)
{
	// Contains an importance value (float) for each node (first) and each triangle (after node values)
	m_hierarchyImportance = std::make_shared<gl::Buffer>(sizeof(float) * (_scene->GetNumTriangles() + _scene->GetNumInnerNodes()), gl::Buffer::IMMUTABLE);
	m_hierarchyImportance->ClearToZero();
	m_hierarchyImportance->BindShaderStorageBuffer(s_hierarchyImportanceBinding);

	gl::MappedUBOView mapView(m_hierarchyImportanceUBOInfo, m_hierarchyImportanceUBO->Map(gl::Buffer::MapType::WRITE, gl::Buffer::MapWriteFlag::NONE));
	mapView["NumInnerNodes"].Set(static_cast<std::int32_t>(_scene->GetNumInnerNodes()));
	mapView["NumTriangles"].Set(static_cast<std::int32_t>(_scene->GetNumTriangles()));
	m_hierarchyImportanceUBO->Unmap();

	// Parent pointer for hierarchy propagation.
	m_sceneParentPointer = std::make_unique<gl::TextureBufferView>(_scene->GetParentBuffer(), gl::TextureBufferFormat::R32I);
	m_sceneParentPointer->BindBuffer(6);

	m_sggxBufferView = std::make_unique<gl::TextureBufferView>(_scene->GetSGGXBuffer(), gl::TextureBufferFormat::R16);
	m_sggxBufferView->BindBuffer(7);
}

void HierarchyImportance::SetScreenSize(const gl::Texture2D& _newBackbuffer)
{
	if (m_hierarchyImportance)
		m_hierarchyImportance->ClearToZero();

	_newBackbuffer.BindImage(0, gl::Texture::ImageAccess::READ_WRITE);
}

void HierarchyImportance::Draw()
{
	// Reset importance on camera movement...
	if(m_rendererSystem.GetIterationCount() == 0)
		m_hierarchyImportance->ClearToZero();

	// Compute triangle importance
	m_hierarchyImpAcquisitionShader.Activate();
	GL_CALL(glDispatchCompute, m_rendererSystem.GetBackbuffer().GetWidth() / m_localSizePathtracer.x, m_rendererSystem.GetBackbuffer().GetHeight() / m_localSizePathtracer.y, 1);
	GL_CALL(glMemoryBarrier, GL_SHADER_STORAGE_BARRIER_BIT);

	// Propagate importance through hierarchy
	UpdateHierarchyNodeImportance();

	// Render
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
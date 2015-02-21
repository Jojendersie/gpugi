#include "hierarchyimportance.hpp"
#include "renderersystem.hpp"
#include "debugrenderer/raytracemeshinfo.hpp"
#include "debugrenderer/hierarchyvisualization.hpp"

#include "../scene/scene.hpp"

#include <glhelper/texture2d.hpp>
#include <glhelper/buffer.hpp>
#include <glhelper/shaderstoragebufferview.hpp>
#include <glhelper/uniformbufferview.hpp>
#include <glhelper/texturebufferview.hpp>
#include <glhelper/utils/flagoperators.hpp>

#include <fstream>

const ei::UVec2 HierarchyImportance::m_localSizePathtracer = ei::UVec2(8, 8);

HierarchyImportance::HierarchyImportance(RendererSystem& _rendererSystem) :
	Renderer(_rendererSystem),
	m_hierarchyImpAcquisitionShader("hierarchyImpAcquisition"),
	m_hierarchyImpTriagVisShader("hierarchyImpTriagVis"),
	m_hierarchyImpPropagationInitShader("hierarchyImpPropagation_Init"),
	m_hierarchyImpPropagationNodeShader("hierarchyImpPropagation_Node")
{
	std::string additionalDefines;
#ifdef SHOW_SPECIFIC_PATHLENGTH
	additionalDefines += "#define SHOW_SPECIFIC_PATHLENGTH " + std::to_string(SHOW_SPECIFIC_PATHLENGTH) + "\n";
#endif

	m_hierarchyImpAcquisitionShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/hierarchy/acquisition.comp", additionalDefines);
	m_hierarchyImpAcquisitionShader.CreateProgram();
	m_hierarchyImpTriagVisShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/hierarchy/trianglevis.comp");
	m_hierarchyImpTriagVisShader.CreateProgram();
	m_hierarchyImpPropagationInitShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/hierarchy/hierarchypropagation_init.comp");
	m_hierarchyImpPropagationInitShader.CreateProgram();
	m_hierarchyImpPropagationNodeShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/hierarchy/hierarchypropagation_nodes.comp");
	m_hierarchyImpPropagationNodeShader.CreateProgram();

	m_hierarchyImportanceUBO.reset(new gl::UniformBufferView(m_hierarchyImpAcquisitionShader, "HierarchyImportanceUBO"));
	m_hierarchyImportanceUBO->BindBuffer(4);

	m_rendererSystem.SetNumInitialLightSamples(128);
}

void HierarchyImportance::SetScene(std::shared_ptr<Scene> _scene)
{
	// Contains an importance value (float) for each node (first) and each triangle (after node values)
	m_hierarchyImportance = std::make_shared<gl::ShaderStorageBufferView>(
							std::make_shared<gl::Buffer>(sizeof(float) * (_scene->GetNumTriangles() + _scene->GetNumInnerNodes()), gl::Buffer::Usage::IMMUTABLE),
							"HierachyImportanceBuffer");
	m_hierarchyImportance->GetBuffer()->ClearToZero();
	m_hierarchyImportance->BindBuffer(s_hierarchyImportanceBinding);

	m_hierarchyImportanceUBO->GetBuffer()->Map();
	(*m_hierarchyImportanceUBO)["NumInnerNodes"].Set(static_cast<std::int32_t>(_scene->GetNumInnerNodes()));
	(*m_hierarchyImportanceUBO)["NumTriangles"].Set(static_cast<std::int32_t>(_scene->GetNumTriangles()));
	m_hierarchyImportanceUBO->GetBuffer()->Unmap();

	// Parent pointer for hierarchy propagation.
	m_sceneParentPointer = std::make_unique<gl::TextureBufferView>(_scene->GetParentBuffer(), gl::TextureBufferFormat::R32I);
	m_sceneParentPointer->BindBuffer(6);
}

void HierarchyImportance::SetScreenSize(const gl::Texture2D& _newBackbuffer)
{
	if (m_hierarchyImportance)
		m_hierarchyImportance->GetBuffer()->ClearToZero();

	_newBackbuffer.BindImage(1, gl::Texture::ImageAccess::READ_WRITE);
}

void HierarchyImportance::Draw()
{
	m_hierarchyImpAcquisitionShader.Activate();
	GL_CALL(glDispatchCompute, m_rendererSystem.GetBackbuffer().GetWidth() / m_localSizePathtracer.x, m_rendererSystem.GetBackbuffer().GetHeight() / m_localSizePathtracer.y, 1);

	GL_CALL(glMemoryBarrier, GL_SHADER_STORAGE_BARRIER_BIT);

	m_hierarchyImpTriagVisShader.Activate();
	GL_CALL(glDispatchCompute, m_rendererSystem.GetBackbuffer().GetWidth() / m_localSizePathtracer.x, m_rendererSystem.GetBackbuffer().GetHeight() / m_localSizePathtracer.y, 1);

	GL_CALL(glMemoryBarrier, GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void HierarchyImportance::UpdateHierarchyNodeImportance()
{
	// First pull importance from the triangles into the nodes.
	// Then pull importance from child nodes, tree layer by tree layer until convergence
	//
	// Why not in a single shader?
	//  Idea would be to perform "pull tries" that finish only if there is importance to pull and busy loop otherwise.
	//  This is not possible with GPUs since a running shader invocation might be waiting for another invocation that was not yet triggered!

	GLuint numBlocks = (m_rendererSystem.GetScene()->GetNumInnerNodes() / 64) + (m_rendererSystem.GetScene()->GetNumInnerNodes() % 64 > 0 ? 1 : 0);

	m_hierarchyImpPropagationInitShader.Activate();
	GL_CALL(glDispatchCompute, numBlocks, 1, 1);
	GL_CALL(glMemoryBarrier, GL_SHADER_STORAGE_BARRIER_BIT);


	gl::ShaderStorageBufferView doneBuffer(std::make_shared<gl::Buffer>(4, gl::Buffer::Usage::MAP_READ | gl::Buffer::Usage::MAP_WRITE), "DoneFlagBuffer");
	doneBuffer.BindBuffer(1);
	m_hierarchyImpPropagationNodeShader.Activate();

	bool changedAnything = false;
	do
	{
		*static_cast<int*>(doneBuffer.GetBuffer()->Map()) = 0;
		doneBuffer.GetBuffer()->Unmap();

		GL_CALL(glDispatchCompute, numBlocks, 1, 1);
		GL_CALL(glMemoryBarrier, GL_SHADER_STORAGE_BARRIER_BIT);

		changedAnything = *static_cast<int*>(doneBuffer.GetBuffer()->Map()) != 0;
		doneBuffer.GetBuffer()->Unmap();
	} while (changedAnything);
}
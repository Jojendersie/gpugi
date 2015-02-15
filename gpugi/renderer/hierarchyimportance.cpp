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

#include <fstream>

const ei::UVec2 HierarchyImportance::m_localSizePathtracer = ei::UVec2(8, 8);

HierarchyImportance::HierarchyImportance(RendererSystem& _rendererSystem) :
	Renderer(_rendererSystem),
	m_hierarchyImpAcquisitionShader("hierarchyImpAcquisition"),
	m_hierarchyImpTriagVisShader("hierarchyImpTriagVis"),
	m_hierarchyImpPropagationShader("hierarchyImpPropagation")
{
	std::string additionalDefines;
#ifdef SHOW_SPECIFIC_PATHLENGTH
	additionalDefines += "#define SHOW_SPECIFIC_PATHLENGTH " + std::to_string(SHOW_SPECIFIC_PATHLENGTH) + "\n";
#endif

	m_hierarchyImpAcquisitionShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/hierarchy/acquisition.comp", additionalDefines);
	m_hierarchyImpAcquisitionShader.CreateProgram();
	m_hierarchyImpTriagVisShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/hierarchy/trianglevis.comp", additionalDefines);
	m_hierarchyImpTriagVisShader.CreateProgram();
	//m_hierarchyImpPropagationShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/hierarchy/trianglevis.comp", additionalDefines);
	//m_hierarchyImpPropagationShader.CreateProgram();

	m_hierarchyImportanceUBO.reset(new gl::UniformBufferView({ &m_hierarchyImpAcquisitionShader, &m_hierarchyImpTriagVisShader }, "HierarchyImportanceUBO"));
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
	m_hierarchyImportance->BindBuffer(0);

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
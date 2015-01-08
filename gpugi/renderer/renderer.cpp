#include "renderer.hpp"

#include <glhelper\texture2d.hpp>
#include <glhelper\texturebuffer.hpp>

#include "..\camera\camera.hpp"
#include "..\scene\scene.hpp"

#include "../utilities/flagoperators.hpp"
#include "../utilities/random.hpp"


Renderer::Renderer() : m_iterationCount(0), m_numInitialLightSamples(0)
{
}

Renderer::~Renderer()
{
}

void Renderer::InitStandardUBOs(const gl::ShaderObject& _reflectionShader)
{
	m_globalConstUBO.Init(_reflectionShader, "GlobalConst");
	m_cameraUBO.Init(_reflectionShader, "Camera");
	m_perIterationUBO.Init(_reflectionShader, "PerIteration", gl::Buffer::Usage::MAP_PERSISTENT | gl::Buffer::Usage::MAP_WRITE | gl::Buffer::Usage::EXPLICIT_FLUSH);
	m_materialUBO.Init(_reflectionShader, "UMaterials");

	m_globalConstUBO.BindBuffer((int)UniformBufferBindings::GLOBALCONST);
	m_cameraUBO.BindBuffer((int)UniformBufferBindings::CAMERA);
	m_perIterationUBO.BindBuffer((int)UniformBufferBindings::PERITERATION);
	m_materialUBO.BindBuffer((int)UniformBufferBindings::MATERIAL);
}

void Renderer::PerIterationBufferUpdate()
{
	m_perIterationUBO["FrameSeed"].Set(WangHash(static_cast<std::uint32_t>(m_iterationCount)));
	m_perIterationUBO.GetBuffer()->Flush();

	// There could be some performance gain in double/triple buffering this buffer.
	m_lightTriangleSampler.GenerateRandomSamples(static_cast<LightTriangleSampler::LightSample*>(m_initialLightSampleBuffer->GetBuffer()->Map()), m_numInitialLightSamples);
	m_initialLightSampleBuffer->GetBuffer()->Flush();
}

void Renderer::SetNumInitialLightSamples(unsigned int _numInitialLightSamples)
{
	m_numInitialLightSamples = _numInitialLightSamples;

	m_initialLightSampleBuffer.reset(new gl::TextureBufferView());
	m_initialLightSampleBuffer->Init(
		std::make_shared<gl::Buffer>(static_cast<std::uint32_t>(sizeof(LightTriangleSampler::LightSample) * m_numInitialLightSamples),
		gl::Buffer::Usage::MAP_WRITE | gl::Buffer::Usage::MAP_PERSISTENT | gl::Buffer::Usage::EXPLICIT_FLUSH), gl::TextureBufferFormat::RGBA32F);
	m_initialLightSampleBuffer->BindBuffer((int)TextureBufferBindings::INITIAL_LIGHTSAMPLES);

	// NumInitialLightSamples is part of the globalconst UBO
	UpdateGlobalConstUBO();
}

void Renderer::SetScene(std::shared_ptr<Scene> _scene)
{
	m_scene = _scene;

	// Bind buffer
	m_triangleBuffer.reset(new gl::TextureBufferView());
	m_triangleBuffer->Init(m_scene->GetTriangleBuffer(), gl::TextureBufferFormat::RGBA32I);

	m_vertexPositionBuffer.reset(new gl::TextureBufferView());
	m_vertexPositionBuffer->Init(m_scene->GetVertexPositionBuffer(), gl::TextureBufferFormat::RGB32F);

	m_vertexInfoBuffer.reset(new gl::TextureBufferView());
	m_vertexInfoBuffer->Init(m_scene->GetVertexInfoBuffer(), gl::TextureBufferFormat::RGBA32F);

	m_hierarchyBuffer.reset(new gl::TextureBufferView());
	m_hierarchyBuffer->Init(m_scene->GetHierarchyBuffer(), gl::TextureBufferFormat::RGBA32F);

	// Bind after creation of all, because bindings are overwritten during construction
	m_triangleBuffer->BindBuffer((int)TextureBufferBindings::TRIANGLES);
	m_vertexPositionBuffer->BindBuffer((int)TextureBufferBindings::VERTEX_POSITIONS);
	m_vertexInfoBuffer->BindBuffer((int)TextureBufferBindings::VERTEX_INFO);
	m_hierarchyBuffer->BindBuffer((int)TextureBufferBindings::HIERARCHY);

	// Upload materials / set textures
	m_materialUBO.GetBuffer()->Map();
	m_materialUBO.Set(&m_scene->GetMaterials().front(), 0, uint32_t(m_scene->GetMaterials().size() * sizeof(Scene::Material)));

	// Set scene for the light triangle sampler
	m_lightTriangleSampler.SetScene(m_scene);


	// Perform a single per iteration update to ensure a valid state.
	PerIterationBufferUpdate();


	m_iterationCount = 0;
	m_backbuffer->ClearToZero(0);
}

void Renderer::SetCamera(const Camera& _camera)
{
	ei::Vec3 camU, camV, camW;
	_camera.ComputeCameraParams(camU, camV, camW);

	m_cameraUBO.GetBuffer()->Map();
	m_cameraUBO["CameraU"].Set(camU);
	m_cameraUBO["CameraV"].Set(camV);
	m_cameraUBO["CameraW"].Set(camW);
	m_cameraUBO["CameraPosition"].Set(_camera.GetPosition());
	m_cameraUBO["PixelArea"].Set(ei::len(ei::cross(camU * 2.0f / m_backbuffer->GetWidth(), camV * 2.0f / m_backbuffer->GetHeight())));
	m_cameraUBO.GetBuffer()->Unmap();

	m_iterationCount = 0;
	m_backbuffer->ClearToZero(0);
}

void Renderer::SetScreenSize(const ei::IVec2& _newSize)
{
	m_backbuffer.reset(new gl::Texture2D(_newSize.x, _newSize.y, gl::TextureFormat::RGBA32F, 1, 0));
	m_backbuffer->ClearToZero(0);

	UpdateGlobalConstUBO();
	m_iterationCount = 0;
}

void Renderer::UpdateGlobalConstUBO() 
{
	if (!m_backbuffer)
		return; // Backbuffer not yet created - this function will be called again if it is ready, so no hurry.

	m_globalConstUBO.GetBuffer()->Map();
	m_globalConstUBO["BackbufferSize"].Set(ei::IVec2(m_backbuffer->GetWidth(), m_backbuffer->GetHeight()));
	m_globalConstUBO["NumInitialLightSamples"].Set(static_cast<std::int32_t>(m_numInitialLightSamples));
	m_globalConstUBO.GetBuffer()->Unmap();
}
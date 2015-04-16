#include "renderersystem.hpp"
#include "renderer.hpp"

#include <glhelper/texture2d.hpp>
#include <glhelper/texturebufferview.hpp>
#include <glhelper/uniformbufferview.hpp>
#include <glhelper/shaderobject.hpp>

#include "../camera/camera.hpp"
#include "../scene/scene.hpp"

#include "../utilities/flagoperators.hpp"
#include "../utilities/random.hpp"
#include "../utilities/logger.hpp"

#include "../control/globalconfig.hpp"

#include "debugrenderer/debugrenderer.hpp"



RendererSystem::RendererSystem() : m_iterationCount(0), m_numInitialLightSamples(0), m_activeRenderer(nullptr), m_activeDebugRenderer(nullptr), m_showLightCachesShader("showLightCaches")
{
	std::unique_ptr<gl::ShaderObject> dummyShader(new gl::ShaderObject("dummy"));
	dummyShader->AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/dummy.comp");
	dummyShader->CreateProgram();
	InitStandardUBOs(*dummyShader);

	PerIterationBufferUpdate();
	m_iterationCount = 0;
	m_renderTime = 0;

	m_showLightCachesShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/showlightcaches.comp");
	m_showLightCachesShader.CreateProgram();
}

RendererSystem::~RendererSystem()
{
	delete m_activeDebugRenderer;
	delete m_activeRenderer;
	GlobalConfig::RemoveParameter("debugrenderer");
}

void RendererSystem::DisableDebugRenderer()
{
	delete m_activeDebugRenderer;
	m_activeDebugRenderer = nullptr;
}


void RendererSystem::InitStandardUBOs(const gl::ShaderObject& _reflectionShader)
{
	m_globalConstUBO = std::make_unique<gl::UniformBufferView>(_reflectionShader, "GlobalConst");
	m_cameraUBO = std::make_unique<gl::UniformBufferView>(_reflectionShader, "Camera");
	m_perIterationUBO = std::make_unique<gl::UniformBufferView>(_reflectionShader, "PerIteration", gl::Buffer::MAP_PERSISTENT | gl::Buffer::MAP_WRITE | gl::Buffer::EXPLICIT_FLUSH);
	m_materialUBO = std::make_unique<gl::UniformBufferView>(_reflectionShader, "UMaterials");

	m_globalConstUBO->BindBuffer((int)UniformBufferBindings::GLOBALCONST);
	m_cameraUBO->BindBuffer((int)UniformBufferBindings::CAMERA);
	m_perIterationUBO->BindBuffer((int)UniformBufferBindings::PERITERATION);
	m_materialUBO->BindBuffer((int)UniformBufferBindings::MATERIAL);
}

void RendererSystem::ResetIterationCount()
{
	m_iterationCount = 0;
	m_renderTime = 0;

	(*m_perIterationUBO)["FrameSeed"].Set(WangHash(static_cast<std::uint32_t>(m_iterationCount)));
	m_perIterationUBO->GetBuffer()->Flush();

	GL_CALL(glMemoryBarrier, GL_TEXTURE_FETCH_BARRIER_BIT);
}

void RendererSystem::PerIterationBufferUpdate(bool _iterationIncrement)
{
	if (_iterationIncrement)
		++m_iterationCount;

	(*m_perIterationUBO)["FrameSeed"].Set(WangHash(static_cast<std::uint32_t>(m_iterationCount)));
	m_perIterationUBO->GetBuffer()->Flush();

	// There could be some performance gain in double/triple buffering this buffer.
	if (m_initialLightSampleBuffer)
	{
		m_lightTriangleSampler.GenerateRandomSamples(static_cast<LightTriangleSampler::LightSample*>(
				m_initialLightSampleBuffer->GetBuffer()->Map(gl::Buffer::MapType::WRITE, gl::Buffer::MapWriteFlag::INVALIDATE_BUFFER)),m_numInitialLightSamples);
		m_initialLightSampleBuffer->GetBuffer()->Flush();
	}

	// Ensure that all future fetches will use the modified data.
	// See https://www.opengl.org/wiki/Memory_Model#Ensuring_visibility
	GL_CALL(glMemoryBarrier, GL_TEXTURE_FETCH_BARRIER_BIT);
}

void RendererSystem::SetNumInitialLightSamples(unsigned int _numInitialLightSamples)
{
	m_numInitialLightSamples = _numInitialLightSamples;

	m_initialLightSampleBuffer = std::make_unique<gl::TextureBufferView>(
		std::make_shared<gl::Buffer>(static_cast<std::uint32_t>(sizeof(LightTriangleSampler::LightSample) * m_numInitialLightSamples),
		gl::Buffer::MAP_WRITE | gl::Buffer::MAP_PERSISTENT | gl::Buffer::EXPLICIT_FLUSH), gl::TextureBufferFormat::RGBA32F);
	m_initialLightSampleBuffer->BindBuffer((int)TextureBufferBindings::INITIAL_LIGHTSAMPLES);

	// NumInitialLightSamples is part of the globalconst UBO
	UpdateGlobalConstUBO();
}

void RendererSystem::SetScene(std::shared_ptr<Scene> _scene)
{
	m_scene = _scene;

	// Bind buffer
	m_triangleBuffer = std::make_unique<gl::TextureBufferView>(m_scene->GetTriangleBuffer(), gl::TextureBufferFormat::RGBA32I);
	m_vertexPositionBuffer = std::make_unique<gl::TextureBufferView>(m_scene->GetVertexPositionBuffer(), gl::TextureBufferFormat::RGB32F);
	m_vertexInfoBuffer = std::make_unique<gl::TextureBufferView>(m_scene->GetVertexInfoBuffer(), gl::TextureBufferFormat::RGBA32F);
	m_hierarchyBuffer = std::make_unique<gl::TextureBufferView>(m_scene->GetHierarchyBuffer(), gl::TextureBufferFormat::RGBA32F);

	// Bind after creation of all, because bindings are overwritten during construction
	m_triangleBuffer->BindBuffer((int)TextureBufferBindings::TRIANGLES);
	m_vertexPositionBuffer->BindBuffer((int)TextureBufferBindings::VERTEX_POSITIONS);
	m_vertexInfoBuffer->BindBuffer((int)TextureBufferBindings::VERTEX_INFO);
	m_hierarchyBuffer->BindBuffer((int)TextureBufferBindings::HIERARCHY);

	// Upload materials / set textures
	m_materialUBO->GetBuffer()->Map(gl::Buffer::MapType::WRITE, gl::Buffer::MapWriteFlag::NONE);
	m_materialUBO->Set(&m_scene->GetMaterials().front(), 0, uint32_t(m_scene->GetMaterials().size() * sizeof(Scene::Material)));

	// Set scene for the light triangle sampler
	m_lightTriangleSampler.SetScene(m_scene);


	// Perform a single per iteration update to ensure a valid state.
	PerIterationBufferUpdate();


	m_iterationCount = 0;
	m_renderTime = 0;
	if (m_backbuffer)
		m_backbuffer->ClearToZero(0);

	if (m_activeRenderer)
		m_activeRenderer->SetScene(m_scene);
	if (m_activeDebugRenderer)
		m_activeDebugRenderer->SetScene(m_scene);
}

void RendererSystem::SetCamera(const Camera& _camera)
{
	m_iterationCount = 0;
	m_renderTime = 0;
	m_camera = _camera;

	if (m_backbuffer)
	{
		ei::Vec3 camU, camV, camW;
		_camera.ComputeCameraParams(camU, camV, camW);
		ei::Mat4x4 viewProjection;
		_camera.ComputeViewProjection(viewProjection);

		m_cameraUBO->GetBuffer()->Map(gl::Buffer::MapType::WRITE, gl::Buffer::MapWriteFlag::NONE);
		(*m_cameraUBO)["CameraU"].Set(camU);
		(*m_cameraUBO)["CameraV"].Set(camV);
		(*m_cameraUBO)["CameraW"].Set(camW);
		(*m_cameraUBO)["CameraPosition"].Set(_camera.GetPosition());
		(*m_cameraUBO)["PixelArea"].Set(ei::len(ei::cross(camU * 2.0f / m_backbuffer->GetWidth(), camV * 2.0f / m_backbuffer->GetHeight())));
		(*m_cameraUBO)["ViewProjection"].Set(viewProjection);
		m_cameraUBO->GetBuffer()->Unmap();

		m_backbuffer->ClearToZero(0);
	}

	if (m_activeRenderer)
		m_activeRenderer->SetCamera(_camera);
}

void RendererSystem::SetScreenSize(const ei::IVec2& _newSize)
{
	m_backbuffer.reset(new gl::Texture2D(_newSize.x, _newSize.y, gl::TextureFormat::RGBA32F, 1, 0));
	m_backbuffer->ClearToZero(0);
	
	UpdateGlobalConstUBO();
	m_iterationCount = 0;
	m_renderTime = 0;

	if (m_activeRenderer)
		m_activeRenderer->SetScreenSize(*m_backbuffer);
}

void RendererSystem::Draw()
{
	if (m_activeRenderer)
	{
		if (m_activeDebugRenderer)
		{
			m_activeDebugRenderer->Draw();
		}
		else
		{
			clock_t begin = clock();
			m_activeRenderer->Draw();
			PerIterationBufferUpdate();
			GL_CALL(glFinish);
			clock_t end = clock();
			m_renderTime += (end - begin) * 1000 / CLOCKS_PER_SEC;
		}
	}
}

void RendererSystem::UpdateGlobalConstUBO() 
{
	if (!m_backbuffer)
		return; // Backbuffer not yet created - this function will be called again if it is ready, so no hurry.

	m_globalConstUBO->GetBuffer()->Map(gl::Buffer::MapType::WRITE, gl::Buffer::MapWriteFlag::NONE);
	(*m_globalConstUBO)["BackbufferSize"].Set(ei::IVec2(m_backbuffer->GetWidth(), m_backbuffer->GetHeight()));
	(*m_globalConstUBO)["NumInitialLightSamples"].Set(static_cast<std::int32_t>(m_numInitialLightSamples));
	m_globalConstUBO->GetBuffer()->Unmap();
}

void RendererSystem::DispatchShowLightCacheShader()
{
	m_showLightCachesShader.Activate();
	GL_CALL(glDispatchCompute, m_numInitialLightSamples, 1, 1);
}
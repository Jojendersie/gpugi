#include "renderersystem.hpp"
#include "renderer.hpp"

#include <glhelper/texture2d.hpp>
#include <glhelper/buffer.hpp>
#include <glhelper/shaderobject.hpp>
#include <glhelper/texturebufferview.hpp>

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
	m_globalConstUBOInfo = _reflectionShader.GetUniformBufferInfo().find("GlobalConst")->second;
	m_globalConstUBO = std::make_unique<gl::Buffer>(m_globalConstUBOInfo.bufferDataSizeByte, gl::Buffer::MAP_WRITE);
	m_globalConstUBO->BindUniformBuffer((int)UniformBufferBindings::GLOBALCONST);

	m_cameraUBOInfo = _reflectionShader.GetUniformBufferInfo().find("Camera")->second;
	m_cameraUBO = std::make_unique<gl::Buffer>(m_cameraUBOInfo.bufferDataSizeByte, gl::Buffer::MAP_WRITE);
	m_cameraUBO->BindUniformBuffer((int)UniformBufferBindings::CAMERA);

	m_perIterationUBOInfo = _reflectionShader.GetUniformBufferInfo().find("PerIteration")->second;
	m_perIterationUBO = std::make_unique<gl::Buffer>(m_perIterationUBOInfo.bufferDataSizeByte, gl::Buffer::MAP_WRITE);
	m_perIterationUBO->BindUniformBuffer((int)UniformBufferBindings::PERITERATION);

	m_materialUBOInfo = _reflectionShader.GetUniformBufferInfo().find("UMaterials")->second;
	m_materialUBO = std::make_unique<gl::Buffer>(m_materialUBOInfo.bufferDataSizeByte, gl::Buffer::MAP_WRITE);
	m_materialUBO->BindUniformBuffer((int)UniformBufferBindings::MATERIAL);
}

void RendererSystem::ResetIterationCount()
{
	m_iterationCount = 0;
	m_renderTime = 0;

	gl::MappedUBOView mappedData(m_perIterationUBOInfo, m_perIterationUBO->Map(gl::Buffer::MapType::WRITE, gl::Buffer::MapWriteFlag::INVALIDATE_BUFFER));
	mappedData["FrameSeed"].Set(WangHash(static_cast<std::uint32_t>(m_iterationCount)));
	m_perIterationUBO->Unmap();

	GL_CALL(glMemoryBarrier, GL_TEXTURE_FETCH_BARRIER_BIT);
}

void RendererSystem::PerIterationBufferUpdate(bool _iterationIncrement)
{
	if (_iterationIncrement)
		++m_iterationCount;

	gl::MappedUBOView mappedData(m_perIterationUBOInfo, m_perIterationUBO->Map(gl::Buffer::MapType::WRITE, gl::Buffer::MapWriteFlag::INVALIDATE_BUFFER));
	mappedData["FrameSeed"].Set(WangHash(static_cast<std::uint32_t>(m_iterationCount)));
	mappedData["IterationCount"].Set(static_cast<std::uint32_t>(m_iterationCount));
	m_perIterationUBO->Unmap();

	// There could be some performance gain in double/triple buffering this buffer.
	if (m_initialLightSampleBuffer)
	{
		m_lightSampler.GenerateRandomSamples(static_cast<LightSampler::LightSample*>(
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
		std::make_shared<gl::Buffer>(static_cast<std::uint32_t>(sizeof(LightSampler::LightSample) * m_numInitialLightSamples),
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
	char* materialUBOData = static_cast<char*>(m_materialUBO->Map(gl::Buffer::MapType::WRITE, gl::Buffer::MapWriteFlag::INVALIDATE_BUFFER));
	memcpy(materialUBOData, m_scene->GetMaterials().data(), uint32_t(m_scene->GetMaterials().size() * sizeof(Scene::Material)));
	m_materialUBO->Unmap();

	// Set scene for the light triangle sampler
	m_lightSampler.SetScene(m_scene);


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


		gl::MappedUBOView mappedData(m_cameraUBOInfo, m_cameraUBO->Map(gl::Buffer::MapType::WRITE, gl::Buffer::MapWriteFlag::INVALIDATE_BUFFER));
		mappedData["CameraU"].Set(camU);
		mappedData["CameraV"].Set(camV);
		mappedData["CameraW"].Set(camW);
		mappedData["CameraPosition"].Set(_camera.GetPosition());
		float pixelArea = ei::len(ei::cross(camU * 2.0f / m_backbuffer->GetWidth(), camV * 2.0f / m_backbuffer->GetHeight()));
		//float tanFOV = tanf(_camera.GetVFov() / 2.0f * (ei::PI / 180.0f));
		float pixelArea2 = 4.0f / (m_backbuffer->GetWidth() * m_backbuffer->GetHeight());
		mappedData["PixelArea"].Set(pixelArea2);
		mappedData["ViewProjection"].Set(viewProjection);
		m_cameraUBO->Unmap();

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
			clock_t begin = clock();
			m_activeDebugRenderer->Draw();
			PerIterationBufferUpdate();
			clock_t end = clock();
			m_renderTime += (end - begin) * 1000 / CLOCKS_PER_SEC;
		}
		else
		{
			clock_t begin = clock();
			m_activeRenderer->Draw();
			PerIterationBufferUpdate();
		//	GL_CALL(glFinish);
			clock_t end = clock();
			m_renderTime += (end - begin) * 1000 / CLOCKS_PER_SEC;
		}
	}
}

void RendererSystem::UpdateGlobalConstUBO() 
{
	if (!m_backbuffer)
		return; // Backbuffer not yet created - this function will be called again if it is ready, so no hurry.

	gl::MappedUBOView mappedData(m_globalConstUBOInfo, m_globalConstUBO->Map(gl::Buffer::MapType::WRITE, gl::Buffer::MapWriteFlag::INVALIDATE_BUFFER));
	mappedData["BackbufferSize"].Set(ei::IVec2(m_backbuffer->GetWidth(), m_backbuffer->GetHeight()));
	mappedData["NumInitialLightSamples"].Set(static_cast<std::int32_t>(m_numInitialLightSamples));
	m_globalConstUBO->Unmap();
}

void RendererSystem::DispatchShowLightCacheShader()
{
	m_showLightCachesShader.Activate();
	GL_CALL(glDispatchCompute, m_numInitialLightSamples, 1, 1);
}
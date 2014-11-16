#include "lightpathtracer.hpp"

#include "../utilities/random.hpp"
#include "../utilities/flagoperators.hpp"

#include "../glhelper/texture2d.hpp"
#include "../glhelper/texturebuffer.hpp"

#include "../Time/Time.h"

#include "../scene/scene.hpp"
#include "../scene/lighttrianglesampler.hpp"

#define LOCAL_SIZE 64

LightPathTracer::LightPathTracer(const Camera& _initialCamera) :
	m_lighttraceShader("lighttracer"),

	m_numInitialLightSamples(256)
//	m_numRaysPerLightSample() Dynamically set depending on resolution, m_numInitialLightSamples and LOCAL_SIZE
{
	m_lighttraceShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/lighttracer.comp");
	m_lighttraceShader.CreateProgram();

	// Uniform buffers
	{
		m_perIterationUBO.Init(m_lighttraceShader, "PerIteration", gl::Buffer::Usage::MAP_PERSISTENT | gl::Buffer::Usage::MAP_WRITE | gl::Buffer::Usage::EXPLICIT_FLUSH);
		m_perIterationUBO.BindBuffer(2);

		m_materialUBO.Init(m_lighttraceShader, "UMaterials");
		m_materialUBO.BindBuffer(3);

		m_lightpathtraceUBO.Init(m_lighttraceShader, "LightPathTrace");
		m_lightpathtraceUBO.BindBuffer(4);
	}

	m_initialLightSampleBuffer.reset(new gl::TextureBufferView());
	m_initialLightSampleBuffer->Init(
		std::make_shared<gl::Buffer>(static_cast<std::uint32_t>(sizeof(LightTriangleSampler::LightSample) * m_numInitialLightSamples),
					gl::Buffer::Usage::MAP_WRITE | gl::Buffer::Usage::MAP_PERSISTENT | gl::Buffer::Usage::EXPLICIT_FLUSH), gl::TextureBufferFormat::RGBA32F);
	m_initialLightSampleBuffer->BindBuffer(4);


	// Additive blending.
	glBlendFunc(GL_ONE, GL_ONE);

	InitStandardUBOs(m_lighttraceShader);
}

void LightPathTracer::SetCamera(const Camera& _camera)
{
	Renderer::SetCamera(_camera);

	m_iterationCount = 0;
	m_backbuffer->ClearToZero(0);
}


void LightPathTracer::SetScene(std::shared_ptr<Scene> _scene)
{
	m_scene = _scene;
	m_lightTriangleSampler.SetScene(m_scene);

	// Bind buffer
	m_triangleBuffer.reset(new gl::TextureBufferView());
	m_triangleBuffer->Init(m_scene->GetTriangleBuffer(), gl::TextureBufferFormat::RGBA32I);
	m_triangleBuffer->BindBuffer(0);

	m_vertexPositionBuffer.reset(new gl::TextureBufferView());
	m_vertexPositionBuffer->Init(m_scene->GetVertexPositionBuffer(), gl::TextureBufferFormat::RGB32F);
	m_vertexPositionBuffer->BindBuffer(1);

	m_vertexInfoBuffer.reset(new gl::TextureBufferView());
	m_vertexInfoBuffer->Init(m_scene->GetVertexInfoBuffer(), gl::TextureBufferFormat::RGBA32F);
	m_vertexInfoBuffer->BindBuffer(2);

	m_hierarchyBuffer.reset(new gl::TextureBufferView());
	m_hierarchyBuffer->Init(m_scene->GetHierarchyBuffer(), gl::TextureBufferFormat::RGBA32F);
	m_hierarchyBuffer->BindBuffer(3);

	// Upload materials / set textures
	m_materialUBO.GetBuffer()->Map();
	m_materialUBO.Set(&m_scene->GetMaterials().front(), 0, uint32_t(m_scene->GetMaterials().size() * sizeof(Scene::Material)));

	// Needs an update, to start with correct light samples.
	PerIterationBufferUpdate();
}

void LightPathTracer::OnResize(const ei::UVec2& _newSize)
{
	Renderer::OnResize(_newSize);
	
	m_iterationCount = 0;
	m_backbuffer->ClearToZero(0);

	m_lockTexture.reset(new gl::Texture2D(_newSize.x, _newSize.y, gl::TextureFormat::R32UI));
	m_lockTexture->ClearToZero(0);
	m_lockTexture->BindImage(1, gl::Texture::ImageAccess::READ_WRITE);

	// Rule: Every block (size = LOCAL_SIZE) should work with the same initial light sample!
	int numPixels = _newSize.x * _newSize.y;
	m_numRaysPerLightSample = std::max(LOCAL_SIZE, (numPixels / m_numInitialLightSamples / LOCAL_SIZE) * LOCAL_SIZE);

	m_lightpathtraceUBO.GetBuffer()->Map();
	m_lightpathtraceUBO["NumInitialLightSamples"].Set(static_cast<std::int32_t>(m_numInitialLightSamples));
	m_lightpathtraceUBO["NumRaysPerLightSample"].Set(static_cast<std::int32_t>(m_numRaysPerLightSample));
	m_lightpathtraceUBO["LightRayPixelWeight"].Set(static_cast<float>(numPixels) / m_numRaysPerLightSample * ei::PI * 2.0f); // Multiplied with 2*PI to account for hemispherical lights!
	m_lightpathtraceUBO.GetBuffer()->Unmap();
}

void LightPathTracer::PerIterationBufferUpdate()
{
	m_perIterationUBO.GetBuffer()->Map();
	m_perIterationUBO["FrameSeed"].Set(WangHash(static_cast<std::uint32_t>(m_iterationCount)));
	m_perIterationUBO["NumLightSamples"].Set(static_cast<std::int32_t>(m_numInitialLightSamples)); // todo
	m_perIterationUBO.GetBuffer()->Flush();

	// There could be some performance gain in double/triple buffering this buffer.
	m_lightTriangleSampler.GenerateRandomSamples(static_cast<LightTriangleSampler::LightSample*>(m_initialLightSampleBuffer->GetBuffer()->Map()), m_numInitialLightSamples);
	m_initialLightSampleBuffer->GetBuffer()->Flush();
}

void LightPathTracer::Draw()
{
	++m_iterationCount;
	
	m_backbuffer->BindImage(0, gl::Texture::ImageAccess::READ_WRITE);

	m_lighttraceShader.Activate();
	GL_CALL(glDispatchCompute, m_numInitialLightSamples * m_numRaysPerLightSample / LOCAL_SIZE, 1, 1);

	PerIterationBufferUpdate();

	// Ensure that all future fetches will use the modified data.
	// See https://www.opengl.org/wiki/Memory_Model#Ensuring_visibility
	GL_CALL(glMemoryBarrier, GL_TEXTURE_FETCH_BARRIER_BIT);
}
#include "referencerenderer.hpp"

#include "../utilities/random.hpp"
#include "../utilities/flagoperators.hpp"

#include "../glhelper/texture2d.hpp"
#include "../glhelper/texturebuffer.hpp"

#include "../Time/Time.h"

#include "../scene/scene.hpp"
#include "../scene/lighttrianglesampler.hpp"

#include <fstream>


ReferenceRenderer::ReferenceRenderer(const Camera& _initialCamera) :
	m_pathtracerShader("pathtracer"),
	m_numInitialLightSamples(16)
{
	m_pathtracerShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/pathtracer.comp");
	m_pathtracerShader.CreateProgram();

	// Save shader binary.
	{
		GLenum binaryFormat;
		auto binary = m_pathtracerShader.GetProgramBinary(binaryFormat);
		char* data = binary.data();
		std::ofstream shaderBinaryFile("pathtracer_binary.txt");
		shaderBinaryFile.write(data, binary.size());
		shaderBinaryFile.close();
	}

	// Uniform buffers
	{
		m_perIterationUBO.Init(m_pathtracerShader, "PerIteration", gl::Buffer::Usage::MAP_PERSISTENT | gl::Buffer::Usage::MAP_WRITE | gl::Buffer::Usage::EXPLICIT_FLUSH);
		m_perIterationUBO.BindBuffer(2);

		m_materialUBO.Init(m_pathtracerShader, "UMaterials");
		m_materialUBO.BindBuffer(3);
	}

	m_initialLightSampleBuffer.reset(new gl::TextureBufferView());
	m_initialLightSampleBuffer->Init(
		std::make_shared<gl::Buffer>(static_cast<std::uint32_t>(sizeof(LightTriangleSampler::LightSample) * m_numInitialLightSamples),
					gl::Buffer::Usage::MAP_WRITE | gl::Buffer::Usage::MAP_PERSISTENT | gl::Buffer::Usage::EXPLICIT_FLUSH), gl::TextureBufferFormat::RGBA32F);
	m_initialLightSampleBuffer->BindBuffer(4);


	// Additive blending.
	glBlendFunc(GL_ONE, GL_ONE);

	InitStandardUBOs(m_pathtracerShader);
	SetCamera(_initialCamera);
}

void ReferenceRenderer::SetCamera(const Camera& _camera)
{
	Renderer::SetCamera(_camera);

	m_iterationCount = 0;
	m_backbuffer->ClearToZero(0);
}


void ReferenceRenderer::SetScene(std::shared_ptr<Scene> _scene)
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

void ReferenceRenderer::OnResize(const ei::UVec2& _newSize)
{
	Renderer::OnResize(_newSize);
	
	m_iterationCount = 0;
	m_backbuffer->ClearToZero(0);
}

void ReferenceRenderer::PerIterationBufferUpdate()
{
	m_perIterationUBO.GetBuffer()->Map();
	m_perIterationUBO["FrameSeed"].Set(WangHash(static_cast<std::uint32_t>(m_iterationCount)));
	m_perIterationUBO["NumLightSamples"].Set(m_numInitialLightSamples); // todo
	m_perIterationUBO.GetBuffer()->Flush();

	// There could be some performance gain in double/triple buffering this buffer.
	m_lightTriangleSampler.GenerateRandomSamples(static_cast<LightTriangleSampler::LightSample*>(m_initialLightSampleBuffer->GetBuffer()->Map()), m_numInitialLightSamples);
	m_initialLightSampleBuffer->GetBuffer()->Flush();
}

void ReferenceRenderer::Draw()
{
	++m_iterationCount;
	
	m_backbuffer->BindImage(0, gl::Texture::ImageAccess::READ_WRITE);

	m_pathtracerShader.Activate();
	GL_CALL(glDispatchCompute, m_backbuffer->GetWidth() / 8, m_backbuffer->GetHeight() / 8, 1);

	PerIterationBufferUpdate();

	// Ensure that all future fetches will use the modified data.
	// See https://www.opengl.org/wiki/Memory_Model#Ensuring_visibility
	GL_CALL(glMemoryBarrier, GL_TEXTURE_FETCH_BARRIER_BIT);
}
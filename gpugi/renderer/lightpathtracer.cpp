#include "lightpathtracer.hpp"
#include <glhelper/texture2d.hpp>

const unsigned int LightPathtracer::m_localSizeLightPathtracer = 8 * 8;

LightPathtracer::LightPathtracer() :
	m_lighttraceShader("lighttracer")
{
	m_lighttraceShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/lighttracer.comp");
	m_lighttraceShader.CreateProgram();

	m_lightpathtraceUBO.Init(m_lighttraceShader, "LightPathTrace");
	m_lightpathtraceUBO.BindBuffer(4);

	InitStandardUBOs(m_lighttraceShader);
	SetNumInitialLightSamples(256);
}

void LightPathtracer::SetScreenSize(const ei::IVec2& _newSize)
{
	Renderer::SetScreenSize(_newSize);
	m_backbuffer->BindImage(0, gl::Texture::ImageAccess::READ_WRITE);
	
	m_iterationCount = 0;

	m_lockTexture.reset(new gl::Texture2D(_newSize.x, _newSize.y, gl::TextureFormat::R32UI));
	m_lockTexture->ClearToZero(0);
	m_lockTexture->BindImage(1, gl::Texture::ImageAccess::READ_WRITE);

	// Rule: Every block (size = m_localSizeLightPathtracer) should work with the same initial light sample!
	int numPixels = _newSize.x * _newSize.y;
	m_numRaysPerLightSample = std::max(m_localSizeLightPathtracer, (numPixels / GetNumInitialLightSamples() / m_localSizeLightPathtracer) * m_localSizeLightPathtracer);

	m_lightpathtraceUBO.GetBuffer()->Map();
	m_lightpathtraceUBO["NumRaysPerLightSample"].Set(static_cast<std::int32_t>(m_numRaysPerLightSample));
	m_lightpathtraceUBO.GetBuffer()->Unmap();
}

void LightPathtracer::Draw()
{
	++m_iterationCount;

	m_lighttraceShader.Activate();
	GL_CALL(glDispatchCompute, GetNumInitialLightSamples() * m_numRaysPerLightSample / m_localSizeLightPathtracer, 1, 1);

	PerIterationBufferUpdate();

	// Ensure that all future fetches will use the modified data.
	// See https://www.opengl.org/wiki/Memory_Model#Ensuring_visibility
	GL_CALL(glMemoryBarrier, GL_TEXTURE_FETCH_BARRIER_BIT);
}
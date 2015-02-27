#include "lightpathtracer.hpp"
#include "renderersystem.hpp"
#include <glhelper/texture2d.hpp>
#include <glhelper/uniformbufferview.hpp>

const unsigned int LightPathtracer::m_localSizeLightPathtracer = 8 * 8;

LightPathtracer::LightPathtracer(RendererSystem& _rendererSystem) :
	Renderer(_rendererSystem),
	m_lighttraceShader("lighttracer"),
	m_numRaysPerLightSample(0)
{
	std::string additionalDefines;
#ifdef SHOW_SPECIFIC_PATHLENGTH
	additionalDefines += "#define SHOW_SPECIFIC_PATHLENGTH " + std::to_string(SHOW_SPECIFIC_PATHLENGTH) + "\n";
#endif

	m_lighttraceShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/lighttracer.comp", additionalDefines);
	m_lighttraceShader.CreateProgram();

	m_lightpathtraceUBO = std::make_unique<gl::UniformBufferView>(m_lighttraceShader, "LightPathTrace");
	m_lightpathtraceUBO->BindBuffer(4);

	_rendererSystem.SetNumInitialLightSamples(256);
}

void LightPathtracer::SetScreenSize(const gl::Texture2D& _newBackbuffer)
{
	_newBackbuffer.BindImage(0, gl::Texture::ImageAccess::READ_WRITE);
	

	m_lockTexture.reset(new gl::Texture2D(_newBackbuffer.GetWidth(), _newBackbuffer.GetHeight(), gl::TextureFormat::R32UI));
	m_lockTexture->ClearToZero(0);
	m_lockTexture->BindImage(1, gl::Texture::ImageAccess::READ_WRITE);

	// Rule: Every block (size = m_localSizeLightPathtracer) should work with the same initial light sample!
	int numPixels = _newBackbuffer.GetWidth() * _newBackbuffer.GetHeight();
	m_numRaysPerLightSample = std::max(m_localSizeLightPathtracer, (numPixels / m_rendererSystem.GetNumInitialLightSamples() / m_localSizeLightPathtracer) * m_localSizeLightPathtracer);

	m_lightpathtraceUBO->GetBuffer()->Map();
	(*m_lightpathtraceUBO)["NumRaysPerLightSample"].Set(static_cast<std::int32_t>(m_numRaysPerLightSample));
	m_lightpathtraceUBO->GetBuffer()->Unmap();
}

void LightPathtracer::Draw()
{
	m_lighttraceShader.Activate();
	GL_CALL(glDispatchCompute, m_rendererSystem.GetNumInitialLightSamples() * m_numRaysPerLightSample / m_localSizeLightPathtracer, 1, 1);

	//m_rendererSystem.DispatchShowLightCacheShader();
}
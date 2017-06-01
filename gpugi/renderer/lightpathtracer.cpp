#include "lightpathtracer.hpp"
#include "renderersystem.hpp"
#include "scene/scene.hpp"
#include <glhelper/texture2d.hpp>
#include <glhelper/buffer.hpp>
#include <algorithm>
#include "application.hpp"

const unsigned int LightPathtracer::m_localSizeLightPathtracer = 8 * 8;

LightPathtracer::LightPathtracer(RendererSystem& _rendererSystem) :
	Renderer(_rendererSystem),
	m_lighttraceShader("lighttracer"),
	m_numRaysPerLightSample(0)
{
	_rendererSystem.SetNumInitialLightSamples(256);
}

void LightPathtracer::SetScene(std::shared_ptr<Scene> _scene)
{
	RecompileShaders(_scene->GetBvhTypeDefineString());
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

	if(m_lightpathtraceUBO)
	{
		gl::MappedUBOView mapView(m_lightpathtraceUBOInfo, m_lightpathtraceUBO->Map(gl::Buffer::MapType::WRITE, gl::Buffer::MapWriteFlag::NONE));
		mapView["NumRaysPerLightSample"].Set(static_cast<std::int32_t>(m_numRaysPerLightSample));
		m_lightpathtraceUBO->Unmap();
	}
}

void LightPathtracer::Draw()
{
	m_lighttraceShader.Activate();
	GL_CALL(glDispatchCompute, m_rendererSystem.GetNumInitialLightSamples() * m_numRaysPerLightSample / m_localSizeLightPathtracer, 1, 1);

	m_rendererSystem.DispatchShowLightCacheShader();
}

void LightPathtracer::RecompileShaders(const std::string& _additionalDefines)
{
	std::string additionalDefines = _additionalDefines;
#ifdef SHOW_SPECIFIC_PATHLENGTH
	additionalDefines += "#define SHOW_SPECIFIC_PATHLENGTH " + std::to_string(SHOW_SPECIFIC_PATHLENGTH) + "\n";
#endif
	additionalDefines += "#define MAX_PATHLENGTH " + std::to_string(GlobalConfig::GetParameter("pathLength")[0].As<int>()) + "\n";

	m_lighttraceShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/lighttracer.comp", additionalDefines);
	m_lighttraceShader.CreateProgram();

	m_lightpathtraceUBOInfo = m_lighttraceShader.GetUniformBufferInfo().find("LightPathTrace")->second;
	m_lightpathtraceUBO = std::make_unique<gl::Buffer>(m_lightpathtraceUBOInfo.bufferDataSizeByte, gl::Buffer::MAP_WRITE);
	m_lightpathtraceUBO->BindUniformBuffer(4);

	gl::MappedUBOView mapView(m_lightpathtraceUBOInfo, m_lightpathtraceUBO->Map(gl::Buffer::MapType::WRITE, gl::Buffer::MapWriteFlag::NONE));
	mapView["NumRaysPerLightSample"].Set(static_cast<std::int32_t>(m_numRaysPerLightSample));
	m_lightpathtraceUBO->Unmap();
}

#include "pixelcachelighttracer.hpp"
#include "renderersystem.hpp"
#include <glhelper/texture2d.hpp>
#include <glhelper/shaderstoragebufferview.hpp>
#include <glhelper/uniformbufferview.hpp>
#include "../utilities/flagoperators.hpp"
#include "../utilities/logger.hpp"

#include <iostream>

const unsigned int PixelCacheLighttracer::m_localSizeLightPathtracer = 8*8;
const ei::UVec2 PixelCacheLighttracer::m_localSizeEyetracer = ei::UVec2(8, 8);

PixelCacheLighttracer::PixelCacheLighttracer(RendererSystem& _rendererSystem) :
	Renderer(_rendererSystem),
	m_lighttraceShader("pixellighttracer"),
	m_eyetraceShader("whittedraytracer"),
	m_numRaysPerLightSample(0)
{
	std::string additionalDefines = "#define STOP_ON_DIFFUSE_BOUNCE\n";
	additionalDefines += "#define SAVE_PIXEL_CACHE\n";

	m_eyetraceShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/whittedraytracer.comp", additionalDefines);
	m_eyetraceShader.CreateProgram();
	m_lighttraceShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/lighttracer_pixca.comp", "");
	m_lighttraceShader.CreateProgram();

	m_lightpathtraceUBO = std::make_unique<gl::UniformBufferView>(m_lighttraceShader, "LightPathTrace");
	m_lightpathtraceUBO->BindBuffer(4);

	m_rendererSystem.SetNumInitialLightSamples(128);
}

void PixelCacheLighttracer::SetScreenSize(const gl::Texture2D& _newBackbuffer)
{
	_newBackbuffer.BindImage(0, gl::Texture::ImageAccess::READ_WRITE);

	// Lock texture for light-camera path connections.
	m_lockTexture.reset(new gl::Texture2D(_newBackbuffer.GetWidth(), _newBackbuffer.GetHeight(), gl::TextureFormat::R32UI));
	m_lockTexture->ClearToZero(0);
	m_lockTexture->BindImage(1, gl::Texture::ImageAccess::READ_WRITE);

	// Rule: Every block (size = m_localSizeLightPathtracer) should work with the same initial light sample!
	int numPixels = _newBackbuffer.GetWidth() * _newBackbuffer.GetHeight();
	m_numRaysPerLightSample = std::max(m_localSizeLightPathtracer, (numPixels / m_rendererSystem.GetNumInitialLightSamples() / m_localSizeLightPathtracer) * m_localSizeLightPathtracer);

	// Set constants ...
	m_lightpathtraceUBO->GetBuffer()->Map();
	(*m_lightpathtraceUBO)["NumRaysPerLightSample"].Set(static_cast<std::int32_t>(m_numRaysPerLightSample));
	(*m_lightpathtraceUBO)["LightRayPixelWeight"].Set(float(numPixels) / 128);//???
	m_lightpathtraceUBO->GetBuffer()->Unmap();

	size_t pixelCacheSizeInBytes = (sizeof(float) * 4 * 4) * _newBackbuffer.GetWidth() * _newBackbuffer.GetHeight();
	m_pixelCache = std::make_unique<gl::ShaderStorageBufferView>(std::make_shared<gl::Buffer>(pixelCacheSizeInBytes, gl::Buffer::Usage::IMMUTABLE), "PixelCache");
	m_pixelCache->BindBuffer(0);			   //?
	m_eyetraceShader.BindSSBO(*m_pixelCache);  //?
	m_lighttraceShader.BindSSBO(*m_pixelCache);//?
	m_pixelCache->GetBuffer()->ClearToZero();
	//float zero = 0.0f;
	//GL_CALL(glClearNamedBufferData, m_pixelCache->GetBuffer()->ClearToZero(), GL_R32F, GL_R, GL_FLOAT, &zero);
	
	//m_pixelCache.reset(new gl::Texture2D(_newBackbuffer.GetWidth(), _newBackbuffer.GetHeight(), gl::TextureFormat::RGB32F));
	//m_pixelCache->BindImage(2, gl::Texture::ImageAccess::READ_WRITE);
}


void PixelCacheLighttracer::Draw()
{
	m_eyetraceShader.Activate();
	GL_CALL(glDispatchCompute, m_rendererSystem.GetBackbuffer().GetWidth() / m_localSizeEyetracer.x, m_rendererSystem.GetBackbuffer().GetHeight() / m_localSizeEyetracer.y, 1);

	GL_CALL(glMemoryBarrier, GL_SHADER_IMAGE_ACCESS_BARRIER_BIT); // Output texture
	GL_CALL(glMemoryBarrier, GL_SHADER_STORAGE_BARRIER_BIT); // Pixel Cache

	m_lighttraceShader.Activate();
	GL_CALL(glDispatchCompute, m_rendererSystem.GetNumInitialLightSamples() * m_numRaysPerLightSample / m_localSizeLightPathtracer, 1, 1);
}
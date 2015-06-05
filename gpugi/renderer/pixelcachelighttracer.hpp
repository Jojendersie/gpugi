#pragma once

#include "renderer.hpp"
#include <glhelper/screenalignedtriangle.hpp>
#include <glhelper/framebufferobject.hpp>
#include <glhelper/shaderobject.hpp>
#include "../scene/lightsampler.hpp"

namespace gl
{
	class Buffer;
}

class PixelCacheLighttracer : public Renderer
{
public:
	PixelCacheLighttracer(RendererSystem& _rendererSystem);
	
	std::string GetName() const override  { return "PixLPT"; }

	void SetScreenSize(const gl::Texture2D& _newBackbuffer) override;

	void Draw() override;


private:
	gl::ShaderObject m_eyetraceShader;
	gl::ShaderObject m_lighttraceShader;

	std::unique_ptr<gl::Texture2D> m_lockTexture;

	std::unique_ptr<gl::Buffer> m_pixelCache;
		
	int m_numRaysPerLightSample;
	gl::UniformBufferMetaInfo m_lightpathtraceUBOInfo;
	std::unique_ptr<gl::Buffer> m_lightpathtraceUBO;

	static const unsigned int m_localSizeLightPathtracer;
	static const ei::UVec2 m_localSizeEyetracer;
};


#pragma once

#include "renderer.hpp"
#include <glhelper/screenalignedtriangle.hpp>
#include <glhelper/framebufferobject.hpp>
#include <glhelper/shaderobject.hpp>
#include "../scene/lightsampler.hpp"

namespace gl
{
	class ShaderStorageBufferView;
	class UniformBufferView;
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

	std::unique_ptr<gl::ShaderStorageBufferView> m_pixelCache;

	int m_numRaysPerLightSample;
	std::unique_ptr<gl::UniformBufferView> m_lightpathtraceUBO;

	static const unsigned int m_localSizeLightPathtracer;
	static const ei::UVec2 m_localSizeEyetracer;
};


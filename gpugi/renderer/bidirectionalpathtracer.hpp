#pragma once

#include "renderer.hpp"
#include "../glhelper/screenalignedtriangle.hpp"
#include "../glhelper/framebufferobject.hpp"
#include "../scene/lighttrianglesampler.hpp"

namespace gl
{
	class ShaderStorageBufferView;
}

// Renderer that uses only light path -> camera connections to render the image.
class BidirectionalPathtracer : public Renderer
{
public:
	BidirectionalPathtracer();
	
	std::string GetName() const override  { return "BPT"; }

	void SetScreenSize(const ei::IVec2& newSize) override;

	void Draw() override;


private:
	gl::ShaderObject m_pathtraceShader;
	gl::ShaderObject m_lighttraceShader;

	std::unique_ptr<gl::Texture2D> m_lockTexture;

	unsigned int m_lightCacheCapacity;
	std::unique_ptr<gl::ShaderStorageBufferView> m_lightCache;

	int m_numRaysPerLightSample;
	gl::UniformBufferView m_lightpathtraceUBO;

	static const unsigned int m_localSizeLightPathtracer;
	static const ei::UVec2 m_localSizePathtracer;
};


#pragma once

#include "renderer.hpp"
#include "../glhelper/screenalignedtriangle.hpp"
#include "../glhelper/framebufferobject.hpp"
#include "../scene/lighttrianglesampler.hpp"

namespace gl
{
	class TextureBufferView;
}

// Renderer that uses only light path -> camera connections to render the image.
class LightPathtracer : public Renderer
{
public:
	LightPathtracer();
	
	std::string GetName() const override  { return "Lighttracer"; }

	void SetScreenSize(const ei::IVec2& newSize) override;

	void Draw() override;

private:
	gl::ShaderObject m_lighttraceShader;

	std::unique_ptr<gl::Texture2D> m_lockTexture;

	int m_numRaysPerLightSample;
	gl::UniformBufferView m_lightpathtraceUBO;

	static const unsigned int m_localSizeLightPathtracer;
};


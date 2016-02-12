#pragma once

#include "renderer.hpp"
#include <glhelper/screenalignedtriangle.hpp>
#include <glhelper/framebufferobject.hpp>
#include <glhelper/shaderobject.hpp>

namespace gl
{
	class TextureBufferView;
}

// Renderer that uses only light path -> camera connections to render the image.
class LightPathtracer : public Renderer
{
public:
	LightPathtracer(RendererSystem& _rendererSystem);
	
	std::string GetName() const override  { return "LPT"; }

	void SetScene(std::shared_ptr<Scene> _scene) override;
	void SetScreenSize(const gl::Texture2D& _newBackbuffer) override;

	void Draw() override;

private:
	void RecompileShaders(const std::string& _additionalDefines);

	gl::ShaderObject m_lighttraceShader;

	std::unique_ptr<gl::Texture2D> m_lockTexture;

	int m_numRaysPerLightSample;

	gl::UniformBufferMetaInfo m_lightpathtraceUBOInfo;
	std::unique_ptr<gl::Buffer> m_lightpathtraceUBO;

	static const unsigned int m_localSizeLightPathtracer;
};


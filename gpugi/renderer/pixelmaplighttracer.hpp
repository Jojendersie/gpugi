#pragma once

#include "renderer.hpp"
#include <glhelper/screenalignedtriangle.hpp>
#include <glhelper/framebufferobject.hpp>
#include <glhelper/shaderobject.hpp>

namespace gl
{
	class TextureBufferView;
}

class PixelMapLighttracer : public Renderer
{
public:
	PixelMapLighttracer(RendererSystem& _rendererSystem);

	std::string GetName() const override { return "IM"; }

	void SetScene(std::shared_ptr<Scene> _scene) override;
	void SetScreenSize(const gl::Texture2D& _newBackbuffer) override;
	void SetEnvironmentMap(std::shared_ptr<gl::TextureCubemap> _envMap) override;

	void Draw() override;

private:
	gl::ShaderObject m_photonTracingShader;
	gl::ShaderObject m_importonDistributionShader;
	static const ei::UVec2 m_localWGSizePhotonShader;
	static const ei::UVec2 m_localWGSizeImportonShader;

	int m_numPhotonsPerLightSample;
	float m_queryRadius;

	gl::UniformBufferMetaInfo m_pixelMapLTUBOInfo;
	std::unique_ptr<gl::Buffer> m_pixelMapLTUBO;
	std::unique_ptr<gl::Buffer> m_pixelMap;
	std::unique_ptr<gl::Buffer> m_pixelMapData;

	std::unique_ptr<gl::Texture2D> m_lockTexture;

	void RecompileShaders(const std::string& _additionalDefines);

	void CreateBuffers();
};

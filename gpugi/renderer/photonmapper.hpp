#pragma once

#include "renderer.hpp"
#include <glhelper/screenalignedtriangle.hpp>
#include <glhelper/framebufferobject.hpp>
#include <glhelper/shaderobject.hpp>

namespace gl
{
	class TextureBufferView;
}

class PhotonMapper : public Renderer
{
public:
	PhotonMapper(RendererSystem& _rendererSystem);
	~PhotonMapper();

	std::string GetName() const override { return "PM"; }

	void SetScene(std::shared_ptr<Scene> _scene) override;
	void SetScreenSize(const gl::Texture2D& _newBackbuffer) override;
	void SetEnvironmentMap(std::shared_ptr<gl::TextureCubemap> _envMap) override;

	void Draw() override;

private:
	gl::ShaderObject m_photonDistributionShader;
	gl::ShaderObject m_gatherShader;
	static const ei::UVec2 m_localWGSizeDistributionShader;
	static const ei::UVec2 m_localWGSizeGatherShader;

	int m_numPhotonsPerLightSample;
	float m_queryRadius;
	float m_currentQueryRadius;
	bool m_progressiveRadius;
	bool m_useStochasticHM;
	uint m_photonMapSize;

	gl::UniformBufferMetaInfo m_photonMapperUBOInfo;
	std::unique_ptr<gl::Buffer> m_photonMapperUBO;
	std::unique_ptr<gl::Buffer> m_photonMap;
	std::unique_ptr<gl::Buffer> m_photonMapData;

	void RecompileShaders(const std::string& _additionalDefines);

	void CreateBuffers();
};

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

class BidirectionalPathtracer : public Renderer
{
public:
	BidirectionalPathtracer(RendererSystem& _rendererSystem);
	
	std::string GetName() const override  { return "BPT"; }

	void SetScene(std::shared_ptr<Scene> _scene) override;
	void SetScreenSize(const gl::Texture2D& _newBackbuffer) override;

	void Draw() override;


private:
	/// Estimates light cache capacity and creates a new cache.
	void CreateLightCacheWithCapacityEstimate();

	void RecompileShaders(const std::string& _additionalDefines);

	gl::ShaderObject m_pathtraceShader;
	gl::ShaderObject m_lighttraceShader;
	gl::ShaderObject m_warmupLighttraceShader;
	gl::ShaderObject m_resetCounterShader;

	std::unique_ptr<gl::Texture2D> m_lockTexture;

	std::unique_ptr<gl::Buffer> m_lightCacheFillCounter;
	std::unique_ptr<gl::Buffer> m_lightCache;

	/// If true, the light cache capacity is not known and needs to be recomputed.
	bool m_needToDetermineNeededLightCacheCapacity;

	int m_numRaysPerLightSample;
	std::unique_ptr<gl::Buffer> m_lightpathtraceUBO;

	static const unsigned int m_localSizeLightPathtracer;
	static const ei::UVec2 m_localSizePathtracer;
};


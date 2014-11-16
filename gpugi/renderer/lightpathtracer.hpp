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
class LightPathTracer : public Renderer
{
public:
	LightPathTracer(const Camera& _initialCamera);
	
	void SetScene(std::shared_ptr<Scene> _scene) override;
	void SetCamera(const Camera& _camera) override;
	void Draw() override;

protected:
	void OnResize(const ei::UVec2& newSize) override;

	void PerIterationBufferUpdate();

private:
	gl::ShaderObject m_lighttraceShader;

	std::unique_ptr<gl::Texture2D> m_lockTexture;

	gl::UniformBufferView m_perIterationUBO;
	gl::UniformBufferView m_materialUBO;
	gl::UniformBufferView m_lightpathtraceUBO;

	std::unique_ptr<gl::TextureBufferView> m_hierarchyBuffer;
	std::unique_ptr<gl::TextureBufferView> m_vertexPositionBuffer;
	std::unique_ptr<gl::TextureBufferView> m_vertexInfoBuffer;
	std::unique_ptr<gl::TextureBufferView> m_triangleBuffer;

	int m_numInitialLightSamples;
	int m_numRaysPerLightSample;
	std::unique_ptr<gl::TextureBufferView> m_initialLightSampleBuffer;
	
	std::shared_ptr<Scene> m_scene;
	LightTriangleSampler m_lightTriangleSampler;
};


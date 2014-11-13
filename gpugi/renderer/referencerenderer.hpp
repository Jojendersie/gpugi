#pragma once

#include "renderer.hpp"
#include "../glhelper/screenalignedtriangle.hpp"
#include "../glhelper/framebufferobject.hpp"
#include "../scene/lighttrianglesampler.hpp"

namespace gl
{
	class TextureBufferView;
}

class ReferenceRenderer : public Renderer
{
public:
	ReferenceRenderer(const Camera& _initialCamera);
	
	void SetScene(std::shared_ptr<Scene> _scene) override;
	void SetCamera(const Camera& _camera) override;
	void Draw() override;

protected:
	void OnResize(const ei::UVec2& newSize) override;

	void PerIterationBufferUpdate();

private:
	gl::ScreenAlignedTriangle m_screenTri;

	gl::FramebufferObject m_backbufferFBO;

	gl::ShaderObject m_pathtracerShader;

	gl::UniformBufferView m_perIterationUBO;
	gl::UniformBufferView m_materialUBO;

	std::unique_ptr<gl::TextureBufferView> m_hierarchyBuffer;
	std::unique_ptr<gl::TextureBufferView> m_vertexPositionBuffer;
	std::unique_ptr<gl::TextureBufferView> m_vertexInfoBuffer;
	std::unique_ptr<gl::TextureBufferView> m_triangleBuffer;

	unsigned int m_numInitialLightSamples;
	std::unique_ptr<gl::TextureBufferView> m_initialLightSampleBuffer;

	
	std::shared_ptr<Scene> m_scene;
	LightTriangleSampler m_lightTriangleSampler;
};


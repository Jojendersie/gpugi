#pragma once

#include "renderer.hpp"
#include "../glhelper/screenalignedtriangle.hpp"
#include "../glhelper/framebufferobject.hpp"

class ReferenceRenderer : public Renderer
{
public:
	ReferenceRenderer(const Camera& _initialCamera);

	void SetCamera(const Camera& _camera) override;
	void Draw() override;

protected:
	void OnResize(const ei::UVec2& newSize) override;

private:
	gl::ScreenAlignedTriangle m_screenTri;

	gl::FramebufferObject m_backbufferFBO;

	gl::ShaderObject m_pathtracerShader;

	gl::UniformBuffer m_globalConstUBO;
	gl::UniformBuffer m_cameraUBO;
	gl::UniformBuffer m_perIterationUBO;
};


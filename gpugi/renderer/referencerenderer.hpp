#pragma once

#include "renderer.hpp"
#include "../glhelper/screenalignedtriangle.hpp"
#include "../glhelper/framebufferobject.hpp"

namespace gl
{
	class StructuredBufferView;
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

private:
	gl::ScreenAlignedTriangle m_screenTri;

	std::unique_ptr<gl::Texture2D> m_iterationBuffer;
	gl::FramebufferObject m_backbufferFBO;

	gl::ShaderObject m_pathtracerShader;
	gl::ShaderObject m_blendShader;

	gl::UniformBufferView m_globalConstUBO;
	gl::UniformBufferView m_cameraUBO;
	gl::UniformBufferView m_perIterationUBO;

	std::unique_ptr<gl::StructuredBufferView> m_hierarchyBuffer;
	std::unique_ptr<gl::StructuredBufferView> m_vertexBuffer;
	std::unique_ptr<gl::StructuredBufferView> m_leafBuffer;

	std::shared_ptr<Scene> m_scene;
};


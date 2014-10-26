#pragma once

#include "renderer.hpp"

class TestRenderer : public Renderer
{
public:
	TestRenderer(const Camera& _initialCamera);

	void SetCamera(const Camera& _camera) override;
	void Draw() override;

private:
	gl::ShaderObject m_testshader;
	gl::UniformBuffer m_testubo;
};


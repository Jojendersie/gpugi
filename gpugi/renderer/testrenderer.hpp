#pragma once

#include "renderer.hpp"

class TestRenderer : public Renderer
{
public:
	TestRenderer(const Camera& initialCamera);

	void SetCamera(const Camera& camera) override;
	void Draw() override;

private:
	gl::ShaderObject testshader;
	gl::UniformBuffer testubo;
};


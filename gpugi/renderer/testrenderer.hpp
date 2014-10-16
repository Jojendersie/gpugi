#pragma once

#include "renderer.hpp"

class TestRenderer : public Renderer
{
public:
	TestRenderer();

	void Draw() override;

private:
	gl::ShaderObject testshader;
	gl::UniformBuffer testubo;
};


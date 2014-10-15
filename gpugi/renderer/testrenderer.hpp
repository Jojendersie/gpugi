#pragma once

#include "renderer.hpp"

class TestRenderer : public Renderer
{
public:
	TestRenderer();

private:
	gl::ShaderObject testshader;
};


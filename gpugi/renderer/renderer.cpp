#include "renderer.hpp"


Renderer::Renderer() :
	backbuffer(1024, 768, gl::TextureFormat::RGBA32F, 1, 0)
{
}


Renderer::~Renderer()
{
}
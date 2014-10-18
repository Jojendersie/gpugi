#include "renderer.hpp"
#include "..\control\globalconfig.hpp"

Renderer::Renderer() :
	backbuffer(static_cast<unsigned int>(GlobalConfig::GetParameter("resolution")[0]),
			   static_cast<unsigned int>(GlobalConfig::GetParameter("resolution")[1]), gl::TextureFormat::RGBA32F, 1, 0)
{
}


Renderer::~Renderer()
{
}
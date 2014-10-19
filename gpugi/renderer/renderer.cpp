#include "renderer.hpp"
#include "..\control\globalconfig.hpp"
#include "..\glhelper\texture2d.hpp"

Renderer::Renderer()
{
	CreateBackbuffer(ei::IVec2(static_cast<int>(GlobalConfig::GetParameter("resolution")[0]), static_cast<int>(GlobalConfig::GetParameter("resolution")[1])));
	GlobalConfig::AddListener("resolution", "renderer", [=](const GlobalConfig::ParameterType& p){ CreateBackbuffer(ei::IVec2(static_cast<int>(p[0]), static_cast<int>(p[1]))); });
}


Renderer::~Renderer()
{
	GlobalConfig::RemovesListener("resolution", "renderer");
}

void Renderer::CreateBackbuffer(const ei::IVec2& resolution)
{
	backbuffer.reset(new gl::Texture2D(resolution.x, resolution.y, gl::TextureFormat::RGBA32F, 1, 0));
}
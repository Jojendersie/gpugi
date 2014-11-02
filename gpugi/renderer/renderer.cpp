#include "renderer.hpp"
#include "..\control\globalconfig.hpp"
#include "..\glhelper\texture2d.hpp"

Renderer::Renderer() : m_iterationCount(0)
{
	CreateBackbuffer(ei::UVec2(GlobalConfig::GetParameter("resolution")[0].As<int>(), GlobalConfig::GetParameter("resolution")[1].As<int>()));
	GlobalConfig::AddListener("resolution", "renderer", [=](const GlobalConfig::ParameterType& p){ this->OnResize(ei::UVec2(p[0].As<int>(), p[1].As<int>())); });
}


Renderer::~Renderer()
{
	GlobalConfig::RemovesListener("resolution", "renderer");
}

void Renderer::CreateBackbuffer(const ei::UVec2& resolution)
{
	m_backbuffer.reset(new gl::Texture2D(resolution.x, resolution.y, gl::TextureFormat::RGBA32F, 1, 0));
}

void Renderer::OnResize(const ei::UVec2& newSize)
{
	CreateBackbuffer(newSize);
}
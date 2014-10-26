#include "renderer.hpp"
#include "..\control\globalconfig.hpp"
#include "..\glhelper\texture2d.hpp"

Renderer::Renderer() : m_iterationCount(0)
{
	CreateBackbuffer(ei::UVec2(static_cast<std::uint32_t>(GlobalConfig::GetParameter("resolution")[0]), static_cast<std::uint32_t>(GlobalConfig::GetParameter("resolution")[1])));
	GlobalConfig::AddListener("resolution", "renderer", [=](const GlobalConfig::ParameterType& p){ this->OnResize(ei::UVec2(static_cast<std::uint32_t>(p[0]), static_cast<std::uint32_t>(p[1]))); });
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
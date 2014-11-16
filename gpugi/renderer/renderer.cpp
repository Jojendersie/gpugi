#include "renderer.hpp"
#include "..\control\globalconfig.hpp"
#include "..\glhelper\texture2d.hpp"
#include "..\camera\camera.hpp"

Renderer::Renderer() : m_iterationCount(0)
{
	CreateBackbuffer(ei::UVec2(GlobalConfig::GetParameter("resolution")[0].As<int>(), GlobalConfig::GetParameter("resolution")[1].As<int>()));
	GlobalConfig::AddListener("resolution", "renderer", [=](const GlobalConfig::ParameterType& p){ this->OnResize(ei::UVec2(p[0].As<int>(), p[1].As<int>())); });
}

Renderer::~Renderer()
{
	GlobalConfig::RemovesListener("resolution", "renderer");
}

void Renderer::InitStandardUBOs(const gl::ShaderObject& _reflectionShader)
{
	m_globalConstUBO.Init(_reflectionShader, "GlobalConst");
	m_globalConstUBO.GetBuffer()->Map();
	m_globalConstUBO["BackbufferSize"].Set(ei::IVec2(m_backbuffer->GetWidth(), m_backbuffer->GetHeight()));
	m_globalConstUBO.BindBuffer(0);

	m_cameraUBO.Init(_reflectionShader, "Camera");
	m_cameraUBO.BindBuffer(1);
}

void Renderer::CreateBackbuffer(const ei::UVec2& resolution)
{
	m_backbuffer.reset(new gl::Texture2D(resolution.x, resolution.y, gl::TextureFormat::RGBA32F, 1, 0));
}

void Renderer::SetCamera(const Camera& _camera)
{
	ei::Vec3 camU, camV, camW;
	_camera.ComputeCameraParams(camU, camV, camW);

	m_cameraUBO.GetBuffer()->Map();
	m_cameraUBO["CameraU"].Set(camU);
	m_cameraUBO["CameraV"].Set(camV);
	m_cameraUBO["CameraW"].Set(camW);
	m_cameraUBO["CameraPosition"].Set(_camera.GetPosition());
	m_cameraUBO.GetBuffer()->Unmap();
}

void Renderer::OnResize(const ei::UVec2& _newSize)
{
	CreateBackbuffer(_newSize);

	m_globalConstUBO.GetBuffer()->Map();
	m_globalConstUBO["BackbufferSize"].Set(ei::IVec2(_newSize.x, _newSize.y));
	m_globalConstUBO.GetBuffer()->Unmap();
}
#include "renderer.hpp"
#include "..\control\globalconfig.hpp"
#include "..\glhelper\texture2d.hpp"
#include "..\camera\camera.hpp"

Renderer::Renderer() : m_iterationCount(0)
{
}

Renderer::~Renderer()
{
}

void Renderer::InitStandardUBOs(const gl::ShaderObject& _reflectionShader)
{
	m_globalConstUBO.Init(_reflectionShader, "GlobalConst");
	m_globalConstUBO.BindBuffer(0);

	m_cameraUBO.Init(_reflectionShader, "Camera");
	m_cameraUBO.BindBuffer(1);
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

void Renderer::SetScreenSize(const ei::IVec2& _newSize)
{
	m_backbuffer.reset(new gl::Texture2D(_newSize.x, _newSize.y, gl::TextureFormat::RGBA32F, 1, 0));

	m_globalConstUBO.GetBuffer()->Map();
	m_globalConstUBO["BackbufferSize"].Set(_newSize);
	m_globalConstUBO.GetBuffer()->Unmap();

	m_backbuffer->ClearToZero(0);
}
#include "camera.hpp"
#include "../utilities/logger.hpp"
#include "../control/globalconfig.hpp"

bool Camera::s_anyCameraConnectedToGlobalConfig = false;

Camera::Camera(const ei::Vec3& _position, const ei::Vec3& _lookat, float _aspectRatio, float _hfov, const ei::Vec3& _up) :
	m_position(_position),
	m_lookat(_lookat),
	m_aspectRatio(_aspectRatio),
	m_yfov(_hfov),
	m_up(_up),
	m_connectedToGlobalConfig(false)
{
}


Camera::~Camera(void)
{
	DisconnectFromGlobalConfig();
}

void Camera::ConnectToGlobalConfig()
{
	if (s_anyCameraConnectedToGlobalConfig)
	{
		LOG_ERROR("Can't connect camera to global config, there is already a camera connected to the global config!");
		return;
	}

	GlobalConfig::AddParameter("cameraPos", { m_position.x, m_position.y, m_position.z }, "Global camera's position");
	GlobalConfig::AddListener("cameraPos", "global camera", [=](const GlobalConfig::ParameterType& p){ this->SetPosition(ei::Vec3(p[0].As<float>(), p[1].As<float>(), p[2].As<float>())); });

	GlobalConfig::AddParameter("cameraLookAt", { m_lookat.x, m_lookat.y, m_lookat.z }, "Global camera's look at");
	GlobalConfig::AddListener("cameraLookAt", "global camera", [=](const GlobalConfig::ParameterType& p){ this->SetLookAt(ei::Vec3(p[0].As<float>(), p[1].As<float>(), p[2].As<float>())); });

	GlobalConfig::AddParameter("cameraFOV", { m_yfov }, "Global camera's vertical FOV");
	GlobalConfig::AddListener("cameraFOV", "global camera", [=](const GlobalConfig::ParameterType& p){ this->SetHFov(p[0].As<float>()); });

	m_connectedToGlobalConfig = true;
	s_anyCameraConnectedToGlobalConfig = true;
}

void Camera::DisconnectFromGlobalConfig()
{
	if (!m_connectedToGlobalConfig)
		return;

	GlobalConfig::RemoveParameter("cameraPos");
	GlobalConfig::RemoveParameter("cameraLookAt");
	GlobalConfig::RemoveParameter("cameraFOV");


	m_connectedToGlobalConfig = false;
	s_anyCameraConnectedToGlobalConfig = false;
}

void Camera::ComputeCameraParams(ei::Vec3& cameraU, ei::Vec3& cameraV, ei::Vec3& cameraW) const
{
	cameraW = ei::normalize(m_lookat - m_position);
	cameraU = ei::normalize(ei::cross(cameraW, m_up));
	cameraV = ei::normalize(ei::cross(cameraU, cameraW));

	float f = tanf(m_yfov / 2.0f * (ei::PI / 180.0f));
	cameraU *= f * m_aspectRatio;
	cameraV *= f;
}

void Camera::ComputeViewProjection(ei::Mat4x4& _viewProjection, float _near, float _far) const
{
	_viewProjection = ei::perspectiveGL(m_yfov * (ei::PI / 180.0f), m_aspectRatio, _near, _far) *
						ei::camera(m_position, m_lookat, m_up);
}
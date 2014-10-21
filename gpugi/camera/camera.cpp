#include "camera.hpp"
#include "../utilities/logger.hpp"
#include "../control/globalconfig.hpp"

bool Camera::s_anyCameraConnectedToGlobalConfig = false;

Camera::Camera(const ei::Vec3& position, const ei::Vec3& lookat, float aspectRatio, float hfov, const ei::Vec3& up) :
	m_position(position),
	m_lookat(lookat),
	m_aspectRatio(aspectRatio),
	m_hfov(hfov),
	m_up(up),
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

	GlobalConfig::AddParameter("cameraPos", { m_position.x, m_position.z, m_position.y }, "Global camera's position");
	GlobalConfig::AddListener("cameraPos", "global camera", [=](const GlobalConfig::ParameterType& p){ this->SetPosition(ei::Vec3(p[0], p[1], p[2])); });

	GlobalConfig::AddParameter("cameraLookAt", { m_lookat.x, m_lookat.z, m_lookat.y }, "Global camera's look at");
	GlobalConfig::AddListener("cameraLookAt", "global camera", [=](const GlobalConfig::ParameterType& p){ this->SetLookAt(ei::Vec3(p[0], p[1], p[2])); });

	GlobalConfig::AddParameter("cameraFOV", { m_hfov }, "Global camera's horizontal FOV");
	GlobalConfig::AddListener("cameraFOV", "global camera", [=](const GlobalConfig::ParameterType& p){ this->SetHFov(p[0]); });

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
	float ulen, vlen, wlen;
	cameraW.x = m_lookat.x - m_position.x;
	cameraW.y = m_lookat.y - m_position.y;  /* Do not normalize W -- it implies focal length */
	cameraW.z = m_lookat.z - m_position.z;

	wlen = sqrtf(ei::dot(cameraW, cameraW));
	cameraU = ei::normalize(ei::cross(cameraW, m_up));
	cameraV = ei::normalize(ei::cross(cameraU, cameraW));
	ulen = wlen * tanf(m_hfov / 2.0f * 3.14159265358979323846f / 180.0f);
	cameraU.x *= ulen;
	cameraU.y *= ulen;
	cameraU.z *= ulen;
	vlen = ulen / m_aspectRatio;
	cameraV.x *= vlen;
	cameraV.y *= vlen;
	cameraV.z *= vlen;
}
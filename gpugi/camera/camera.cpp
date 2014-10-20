#include "camera.hpp"
#include "../utilities/logger.hpp"

bool Camera::s_anyCameraConnectedToGlobalConfig = false;

Camera::Camera(const ei::Vec3& position, const ei::Vec3& lookat, float aspectRatio, float hfov, const ei::Vec3& up) :
	m_position(position),
	m_lookat(lookat),
	m_aspectRatio(aspectRatio),
	m_hfov(hfov),
	m_up(up),
	m_connectedToGlobalConfig(false)
{
	UpdateCameraParams();
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

	// TODO

	m_connectedToGlobalConfig = true;
	s_anyCameraConnectedToGlobalConfig = true;
}

void Camera::DisconnectFromGlobalConfig()
{
	if (!m_connectedToGlobalConfig)
		return;

	// TODO

	m_connectedToGlobalConfig = false;
	s_anyCameraConnectedToGlobalConfig = false;
}

void Camera::UpdateCameraParams()
{
	float ulen, vlen, wlen;
	m_cameraW.x = m_lookat.x - m_position.x;
	m_cameraW.y = m_lookat.y - m_position.y;  /* Do not normalize W -- it implies focal length */
	m_cameraW.z = m_lookat.z - m_position.z;

	wlen = sqrtf(ei::dot(m_cameraW, m_cameraW));
	m_cameraU = ei::normalize(ei::cross(m_cameraW, m_up));
	m_cameraV = ei::normalize(ei::cross(m_cameraU, m_cameraW));
	ulen = wlen * tanf(m_hfov / 2.0f * 3.14159265358979323846f / 180.0f);
	m_cameraU.x *= ulen;
	m_cameraU.y *= ulen;
	m_cameraU.z *= ulen;
	vlen = ulen / m_aspectRatio;
	m_cameraV.x *= vlen;
	m_cameraV.y *= vlen;
	m_cameraV.z *= vlen;
}
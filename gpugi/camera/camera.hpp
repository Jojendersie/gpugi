#pragma once

#include <ei/matrix.hpp>

class Camera
{
public:
	Camera(const ei::Vec3& position, const ei::Vec3& lookat, float aspectRatio, float hfov = 60.0f, const ei::Vec3& up = ei::Vec3(0.0f, 1.0f, 0.0f));
	virtual ~Camera();

	/// Connects parameters to global config, will also listen to resolution.
	/// Only one camera can be connected to the global config.
	/// Registers: cameraPos, cameraLookAt, cameraFOV
	void ConnectToGlobalConfig();
	void DisconnectFromGlobalConfig();

	const ei::Vec3& GetPosition() const  { return m_position; }
	const ei::Vec3& GetLookAt() const  { return m_lookat; }

	void SetAspectRatio(float aspect)				{ m_aspectRatio = aspect; UpdateCameraParams(); }
	void SetHFov(float hfov)						{ m_hfov = hfov; UpdateCameraParams(); }
	void SetPosition(const ei::Vec3& position)		{ m_position = position; UpdateCameraParams(); }
	void SetLookAt(const ei::Vec3& lookat)			{ m_lookat = lookat; UpdateCameraParams(); }
	
	const ei::Vec3& GetUp() const  { return m_up; }
	const float GetHFov() const  { return m_hfov; }
	const float GetAspectRatio() const  { return m_aspectRatio; }

	const ei::Vec3& GetCameraU() const  { return m_cameraU; }
	const ei::Vec3& GetCameraV() const  { return m_cameraV; }
	const ei::Vec3& GetCameraW() const  { return m_cameraW; }


protected:
	ei::Vec3 m_position;
	ei::Vec3 m_lookat;
	ei::Vec3 m_up;
	float m_hfov;
	float m_aspectRatio;

	void UpdateCameraParams();

private:
	ei::Vec3 m_cameraU, m_cameraV, m_cameraW;

	static bool s_anyCameraConnectedToGlobalConfig;
	bool m_connectedToGlobalConfig;
};


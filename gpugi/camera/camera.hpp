#pragma once

#include <ei/vector.hpp>

class Camera
{
public:
	Camera();
	Camera(Camera& _camera);
	Camera(const ei::Vec3& _position, const ei::Vec3& _lookat, float _aspectRatio, float _verticalFOV = 60.0f, const ei::Vec3& _up = ei::Vec3(0.0f, 1.0f, 0.0f));
	virtual ~Camera();

	/// Connects parameters to global config, will also listen to resolution.
	/// Only one camera can be connected to the global config.
	/// Registers: cameraPos, cameraLookAt, cameraFOV
	void ConnectToGlobalConfig();
	void DisconnectFromGlobalConfig();

	const ei::Vec3& GetPosition() const  { return m_position; }
	const ei::Vec3& GetLookAt() const  { return m_lookat; }

	virtual void SetAspectRatio(float aspect)				{ m_aspectRatio = aspect; }
	virtual void SetVFov(float vfov)						{ m_yfov = vfov; }
	virtual void SetPosition(const ei::Vec3& position)		{ m_position = position; }
	virtual void SetLookAt(const ei::Vec3& lookat)			{ m_lookat = lookat; }
	
	const ei::Vec3& GetUp() const  { return m_up; }
	const float GetVFov() const  { return m_yfov; }
	const float GetAspectRatio() const  { return m_aspectRatio; }

	void ComputeCameraParams(ei::Vec3& cameraU, ei::Vec3& cameraV, ei::Vec3& cameraW) const;
	void ComputeViewProjection(ei::Mat4x4& _viewProjection, float _near = 0.1f, float _far = 10000.0f) const;

protected:
	ei::Vec3 m_position;
	ei::Vec3 m_lookat;
	ei::Vec3 m_up;
	float m_yfov;
	float m_aspectRatio;

	bool IsConnectedToGlobalConfig() const { return m_connectedToGlobalConfig; }

private:
	static bool s_anyCameraConnectedToGlobalConfig;
	bool m_connectedToGlobalConfig;
};


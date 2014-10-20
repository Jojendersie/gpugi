#pragma once

#include "camera.hpp"
#include "../Time/Time.h"

struct GLFWwindow;

class InteractiveCamera : public Camera
{
public:
	InteractiveCamera(GLFWwindow* window, const Camera& camera);
	InteractiveCamera(GLFWwindow* window, const ei::Vec3& position, const ei::Vec3& lookat, float aspectRatio, float hfov = 60.0f, const ei::Vec3& up = ei::Vec3(0.0f, 1.0f, 0.0f));

	void Reset(const Camera& camera);

	// Returns true if there was an update
	bool Update(ezTime timeSinceLastFrame);

private:
	void RotFromLookat();

	GLFWwindow* m_window;
	double m_rotX, m_rotY;
	float m_rotSpeed, m_moveSpeed;
	double m_lastMousePosX, m_lastMousePosY;
};


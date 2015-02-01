#pragma once

#include "camera.hpp"
#include "../Time/Time.h"

struct GLFWwindow;

class InteractiveCamera : public Camera
{
public:
	InteractiveCamera(GLFWwindow* window, const Camera& camera);
	InteractiveCamera(GLFWwindow* window, const ei::Vec3& position, const ei::Vec3& lookat, float aspectRatio, float hfov = 60.0f, const ei::Vec3& up = ei::Vec3(0.0f, 1.0f, 0.0f));

	virtual void SetAspectRatio(float aspect) override				{ m_dirty = true; m_aspectRatio = aspect; }
	virtual void SetVFov(float hfov) override						{ m_dirty = true; m_yfov = hfov; }
	virtual void SetPosition(const ei::Vec3& position) override		{ m_dirty = true; m_position = position; }
	virtual void SetLookAt(const ei::Vec3& lookat) override			{ m_dirty = true; m_lookat = lookat; }

	void Reset(const Camera& camera);

	/// Returns true if any values changed either via a setter or within this function.
	/// Dirty state is reset after calling this function.
	bool Update(ezTime timeSinceLastFrame);

private:
	GLFWwindow* m_window;
	float m_rotSpeed, m_moveSpeed;
	double m_lastMousePosX, m_lastMousePosY;

	bool m_dirty;
};


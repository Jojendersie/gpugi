#include "interactivecamera.hpp"
#include <GLFW/glfw3.h>
#include "../control/globalconfig.hpp"

InteractiveCamera::InteractiveCamera(GLFWwindow* window, const Camera& camera) :
	Camera(camera.GetPosition(), camera.GetLookAt(), camera.GetAspectRatio(), camera.GetHFov(), camera.GetUp()),
	m_window(m_window),
	m_rotSpeed(0.01f),
	m_moveSpeed(16.0f),
	m_rotX(0), m_rotY(0), m_dirty(true)
{
	RotFromLookat();
}

void InteractiveCamera::Reset(const Camera& camera)
{
	m_position = camera.GetPosition();
	m_lookat = camera.GetLookAt();
	m_aspectRatio = camera.GetAspectRatio();
	m_hfov = camera.GetHFov();
	m_up = camera.GetUp();

	RotFromLookat();
}

InteractiveCamera::InteractiveCamera(GLFWwindow* window, const ei::Vec3& position, const ei::Vec3& lookat, float aspectRatio, float hfov, const ei::Vec3& up) : 
	Camera(position, lookat, aspectRatio, hfov, up),
	m_window(window),
	m_rotSpeed(0.01f),
	m_moveSpeed(16.0f),
	m_rotX(0), m_rotY(0)
{
	RotFromLookat();
}

void InteractiveCamera::RotFromLookat()
{
	ei::Vec3 cameraDirection = ei::normalize(m_lookat - m_position);

	m_rotY = asinf(cameraDirection.y);
	m_rotX = asinf(cameraDirection.x / cosf((float)m_rotY));
}

bool InteractiveCamera::Update(ezTime timeSinceLastFrame)
{
	double newMousePosX, newMousePosY;
	glfwGetCursorPos(m_window, &newMousePosX, &newMousePosY);

	if(glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT))
	{
		m_rotX += -m_rotSpeed * (newMousePosX - m_lastMousePosX);
		m_rotY += m_rotSpeed * (newMousePosY - m_lastMousePosY);

		float scaledMoveSpeed = m_moveSpeed;
		if(glfwGetKey(m_window, GLFW_KEY_RIGHT_SHIFT))
			scaledMoveSpeed *= 10.0f;

		float forward = (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(m_window, 'w') == GLFW_PRESS) ? 1.0f : 0.0f;
		float back	  = (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(m_window, 's') == GLFW_PRESS) ? 1.0f : 0.0f;
		float left	  = (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(m_window, 'a') == GLFW_PRESS) ? 1.0f : 0.0f;
		float right	  = (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(m_window, 'd') == GLFW_PRESS) ? 1.0f : 0.0f;

		ei::Vec3 cameraDirection = ei::Vec3(static_cast<float>(sin(m_rotX) * cos(m_rotY)), static_cast<float>(sin(m_rotY)), static_cast<float>(cos(m_rotX) * cos(m_rotY)));
		ei::Vec3 cameraLeft = ei::cross(cameraDirection, ei::Vec3(0, 1, 0));

		ei::Vec3 oldPosition = m_position;
		ei::Vec3 oldLookat = m_lookat;
		m_position = m_position + ((forward - back) * cameraDirection + (right - left) * cameraLeft) * scaledMoveSpeed * static_cast<float>(timeSinceLastFrame.GetSeconds());
		m_lookat = m_position + cameraDirection;

		
		
		m_dirty |= oldPosition.x != m_position.x || oldPosition.y != m_position.y || oldPosition.z != m_position.z ||
					oldLookat.x != m_lookat.x || oldLookat.y != m_lookat.y || oldLookat.z != m_lookat.z;

		if (IsConnectedToGlobalConfig() && m_dirty)
		{
			GlobalConfig::SetParameter("cameraPos", { m_position.x, m_position.y, m_position.z });
			GlobalConfig::SetParameter("cameraLookAt", { m_lookat.x, m_lookat.y, m_lookat.z });
		}
	}

	m_lastMousePosX = newMousePosX;
	m_lastMousePosY = newMousePosY;

	bool outDirty = m_dirty;
	m_dirty = false;
	return outDirty;
}
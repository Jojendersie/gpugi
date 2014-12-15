#include "interactivecamera.hpp"
#include <GLFW/glfw3.h>
#include "../control/globalconfig.hpp"

InteractiveCamera::InteractiveCamera(GLFWwindow* window, const Camera& camera) :
	Camera(camera.GetPosition(), camera.GetLookAt(), camera.GetAspectRatio(), camera.GetHFov(), camera.GetUp()),
	m_window(m_window),
	m_rotSpeed(0.01f),
	m_moveSpeed(16.0f),
	m_dirty(true)
{
}

void InteractiveCamera::Reset(const Camera& camera)
{
	m_position = camera.GetPosition();
	m_lookat = camera.GetLookAt();
	m_aspectRatio = camera.GetAspectRatio();
	m_hfov = camera.GetHFov();
	m_up = camera.GetUp();
}

InteractiveCamera::InteractiveCamera(GLFWwindow* window, const ei::Vec3& position, const ei::Vec3& lookat, float aspectRatio, float hfov, const ei::Vec3& up) : 
	Camera(position, lookat, aspectRatio, hfov, up),
	m_window(window),
	m_rotSpeed(0.01f),
	m_moveSpeed(4.0f)
{
}


bool InteractiveCamera::Update(ezTime timeSinceLastFrame)
{
	double newMousePosX, newMousePosY;
	glfwGetCursorPos(m_window, &newMousePosX, &newMousePosY);

	if(glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT))
	{
		ei::Vec3 cameraDirection = ei::normalize(m_lookat - m_position);
		double rotY = asin(cameraDirection.y);
		double rotX = atan2(cameraDirection.x, cameraDirection.z); 
		rotX += -m_rotSpeed * (newMousePosX - m_lastMousePosX);
		rotY += m_rotSpeed * (newMousePosY - m_lastMousePosY);

		float scaledMoveSpeed = m_moveSpeed;
		if (glfwGetKey(m_window, GLFW_KEY_RIGHT_SHIFT) || glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT))
			scaledMoveSpeed *= 10.0f;

		float forward = (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) ? 1.0f : 0.0f;
		float back = (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) ? 1.0f : 0.0f;
		float left = (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) ? 1.0f : 0.0f;
		float right = (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) ? 1.0f : 0.0f;

		cameraDirection = ei::Vec3(static_cast<float>(sin(rotX) * cos(rotY)), static_cast<float>(sin(rotY)), static_cast<float>(cos(rotX) * cos(rotY)));
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
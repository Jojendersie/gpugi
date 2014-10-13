#pragma once

#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

struct GLFWwindow;

/// A GLFW based output window
class OutputWindow
{
public:
	OutputWindow(unsigned int width, unsigned int height);
	~OutputWindow();

	/// Polls events to keep the window responsive.
	void PollWindowEvents();

	/// Returns false if the window was or wants to be closed.
	bool IsWindowAlive();

	bool WasButtonPressed(unsigned int glfwKey);

	/// Returns GLFW window.
	GLFWwindow* GetGLFWWindow() { return window; }

	void Present();

	void SetTitle(const std::string& windowTitle);

private:
	void GetGLFWKeystates();
	
	GLFWwindow* window;

	int oldGLFWKeystates[GLFW_KEY_LAST];
};


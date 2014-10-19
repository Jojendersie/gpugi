#pragma once

#include "glhelper/shaderobject.hpp"
#include <GLFW/glfw3.h>
#include <ei/matrix.hpp>

struct GLFWwindow;
namespace gl
{
	class Texture2D;
	class ScreenAlignedTriangle;
}

/// A GLFW based output window
class OutputWindow
{
public:
	OutputWindow();
	~OutputWindow();

	void ChangeWindowSize(const ei::IVec2& newResolution);
	
	/// Polls events to keep the window responsive.
	void PollWindowEvents();

	/// Returns false if the window was or wants to be closed.
	bool IsWindowAlive();

	bool WasButtonPressed(unsigned int glfwKey);

	/// Returns GLFW window.
	GLFWwindow* GetGLFWWindow() { return window; }

	void DisplayHDRTexture(gl::Texture2D& texture);

	void Present();

	void SetTitle(const std::string& windowTitle);

private:
	void GetGLFWKeystates();
	
	GLFWwindow* window;
	gl::ShaderObject displayHDR;
	gl::ScreenAlignedTriangle* screenTri;

	int oldGLFWKeystates[GLFW_KEY_LAST];
};


#include "outputwindow.hpp"

#include "glhelper/gl.hpp"

#include <iostream>
#include <cassert>
#include <memory>

static void ErrorCallbackGLFW(int error, const char* description)
{
	std::cerr << "GLFW error, code " << error << " desc: \"" << description << "\"" << std::endl;
}

OutputWindow::OutputWindow(unsigned int width, unsigned int height)
{
	if (!glfwInit())
		throw std::exception("GLFW init failed");

	glfwSetErrorCallback(ErrorCallbackGLFW);

	window = glfwCreateWindow(width, height, "<Add fancy title here>", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		throw std::exception("Failed to create glfw window!");
	}

	glfwMakeContextCurrent(window);

	// Init glew now since the GL context is ready.
	GLenum err = glewInit();
	if (err != GLEW_OK)
		throw std::string("Error: ") + reinterpret_cast<const char*>(glewGetErrorString(err));

#ifdef _DEBUG
	gl::ActivateGLDebugOutput(gl::DebugSeverity::MEDIUM);
#endif

	GetGLFWKeystates();
}

OutputWindow::~OutputWindow(void)
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

/// Polls events to keep the window responsive.
void OutputWindow::PollWindowEvents()
{
	GetGLFWKeystates();
	glfwPollEvents();
}

bool OutputWindow::IsWindowAlive()
{
	return !glfwWindowShouldClose(window);
}

void OutputWindow::GetGLFWKeystates()
{
	for(unsigned int i=0; i< GLFW_KEY_LAST; ++i)
		oldGLFWKeystates[i] = glfwGetKey(window, i);
}

bool OutputWindow::WasButtonPressed(unsigned int glfwKey)
{
	if(glfwKey >= GLFW_KEY_LAST)
		return false;

	return glfwGetKey(window, glfwKey) == GLFW_PRESS && oldGLFWKeystates[glfwKey] == GLFW_RELEASE;
}

void OutputWindow::SetTitle(const std::string& windowTitle)
{
	glfwSetWindowTitle(window, windowTitle.c_str());
}

void OutputWindow::Present()
{
	glfwSwapBuffers(window);
}

#include "OutputWindow.h"

#include <iostream>
#include <cassert>
#include <memory>

static void ErrorCallbackGLFW(int error, const char* description)
{
	std::cerr << "GLFW error, code " << error << " desc: \"" << description << "\"" << std::endl;
}

static void APIENTRY GLDebugOutput(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	GLvoid* userParam
	)
{
	std::string debSource, debType, debSev;

	if(source == GL_DEBUG_SOURCE_API_ARB)
		debSource = "OpenGL";
	else if(source == GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB)
		debSource = "Windows";
	else if(source == GL_DEBUG_SOURCE_SHADER_COMPILER_ARB)
		debSource = "Shader Compiler";
	else if(source == GL_DEBUG_SOURCE_THIRD_PARTY_ARB)
		debSource = "Third Party";
	else if(source == GL_DEBUG_SOURCE_APPLICATION_ARB)
		debSource = "Application";
	else if(source == GL_DEBUG_SOURCE_OTHER_ARB)
		debSource = "Other";

	if(type == GL_DEBUG_TYPE_ERROR_ARB)
	{
		debType = "error";
	}
	else if(type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB)
	{
		debType = "deprecated behavior";
	}
	else if(type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB)
	{
		debType = "undefined behavior";
	}
	else if(type == GL_DEBUG_TYPE_PORTABILITY_ARB)
	{
		debType = "portability";
	}
	else if(type == GL_DEBUG_TYPE_PERFORMANCE_ARB)
	{
		debType = "performance";
	}
	else if(type == GL_DEBUG_TYPE_OTHER_ARB)
	{
		debType = "message";
	}

	if(severity == GL_DEBUG_SEVERITY_MEDIUM_ARB)
		debSev = "medium";
	else if(severity == GL_DEBUG_SEVERITY_LOW_ARB)
		debSev = "low";

	std::cerr << debSource << ": " << debType << "(" << debSev << ") " << id << ": " << message << std::endl;
}

void ActivateDebugOutput()
{
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

//	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, NULL, GL_TRUE);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, NULL, GL_TRUE);
//	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, NULL, GL_TRUE);

	glDebugMessageCallback(&GLDebugOutput, NULL);
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
	ActivateDebugOutput();
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

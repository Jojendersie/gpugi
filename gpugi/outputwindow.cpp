#include "outputwindow.hpp"

#include "glhelper/texture2d.hpp"
#include "glhelper/screenalignedtriangle.hpp"

#include "utilities/logger.hpp"

#include <iostream>
#include <cassert>
#include <memory>
#include <string>

static void ErrorCallbackGLFW(int error, const char* description)
{
	LOG_ERROR("GLFW error, code " + std::to_string(error) + " desc: \"" + description + "\"");
}

OutputWindow::OutputWindow(unsigned int width, unsigned int height) :
	displayHDR("displayHDR")
{
	if (!glfwInit())
		throw std::exception("GLFW init failed");

	glfwSetErrorCallback(ErrorCallbackGLFW);

	// OpenGL 4.5 with forward compatibility (removed deprecated stuff)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// OpenGL Debug context.
#ifdef _DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

	window = glfwCreateWindow(width, height, "<Add fancy title here>", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		throw std::exception("Failed to create glfw window!");
	}

	glfwMakeContextCurrent(window);

	// Init glew now since the GL context is ready.
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (err != GLEW_OK)
		throw std::string("Error: ") + reinterpret_cast<const char*>(glewGetErrorString(err));

#ifdef _DEBUG
	gl::ActivateGLDebugOutput(gl::DebugSeverity::MEDIUM);
#endif

	// There must be a non-zero VAO at all times.
	// http://stackoverflow.com/questions/21767467/glvertexattribpointer-raising-impossible-gl-invalid-operation
	GLuint vao;
	GL_CALL(glGenVertexArrays, 1, &vao);
	GL_CALL(glBindVertexArray, vao);

	GetGLFWKeystates();



	displayHDR.AddShaderFromFile(gl::ShaderObject::ShaderType::VERTEX, "shader/screenTri.vert");
	displayHDR.AddShaderFromFile(gl::ShaderObject::ShaderType::FRAGMENT, "shader/displayHDR.frag");
	displayHDR.CreateProgram();

	screenTri = new gl::ScreenAlignedTriangle();
}

OutputWindow::~OutputWindow(void)
{
	SAFE_DELETE(screenTri);

	glfwDestroyWindow(window);
	window = nullptr;
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

void OutputWindow::DisplayHDRTexture(gl::Texture2D& texture)
{
	texture.Bind(0);
	displayHDR.Activate();
	screenTri->Draw();
}

void OutputWindow::Present()
{
	glfwSwapBuffers(window);
}

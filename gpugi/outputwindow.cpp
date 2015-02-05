#include "outputwindow.hpp"

#include "glhelper/texture2d.hpp"
#include "glhelper/screenalignedtriangle.hpp"

#include "control/globalconfig.hpp"
#include "utilities/logger.hpp"

#include <iostream>
#include <cassert>
#include <memory>
#include <string>


static void ErrorCallbackGLFW(int error, const char* description)
{
	LOG_ERROR("GLFW error, code " + std::to_string(error) + " desc: \"" + description + "\"");
}

OutputWindow::OutputWindow() :
	displayHDR("displayHDR")
{
	if (!glfwInit())
		throw std::exception("GLFW init failed");

	glfwSetErrorCallback(ErrorCallbackGLFW);

	// OpenGL 4.5 with forward compatibility (removed deprecated stuff)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// OpenGL Debug context.
#ifdef _DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

	// Setup with a default resolution.
	int width = 1024;
	int height = 768;
	GlobalConfig::AddParameter("resolution", { width, height }, "The window's width and height.");

	window = glfwCreateWindow(width, height, "<Add fancy title here>", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		throw std::exception("Failed to create glfw window!");
	}

	GlobalConfig::AddListener("resolution", "outputwindow", [=](const GlobalConfig::ParameterType& p){ ChangeWindowSize(ei::IVec2(p[0].As<int>(), p[1].As<int>())); });
	GlobalConfig::AddParameter("srgb", { true }, "If true the output window will perform an srgb conversion. Does not affect (hdr)screenshots!");

	glfwMakeContextCurrent(window);

	// Init glew now since the GL context is ready.
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (err != GLEW_OK)
		throw std::string("Error: ") + reinterpret_cast<const char*>(glewGetErrorString(err));

	// glew has a problem with GLCore and gives sometimes a GL_INVALID_ENUM
	// http://stackoverflow.com/questions/10857335/opengl-glgeterror-returns-invalid-enum-after-call-to-glewinit
	// Ignore it:
	glGetError();

#ifdef _DEBUG
	gl::ActivateGLDebugOutput(gl::DebugSeverity::LOW);
#endif
	
	// Disable V-Sync
	glfwSwapInterval(0);

	GL_CALL(glViewport, 0, 0, width, height);

	// There must be a non-zero VAO at all times.
	// http://stackoverflow.com/questions/21767467/glvertexattribpointer-raising-impossible-gl-invalid-operation
	GLuint vao;
	GL_CALL(glGenVertexArrays, 1, &vao);
	GL_CALL(glBindVertexArray, vao);

	GetGLFWKeystates();



	displayHDR.AddShaderFromFile(gl::ShaderObject::ShaderType::VERTEX, "shader/utils/screenTri.vert");
	displayHDR.AddShaderFromFile(gl::ShaderObject::ShaderType::FRAGMENT, "shader/displayHDR.frag");
	displayHDR.CreateProgram();

	screenTri = new gl::ScreenAlignedTriangle();
}

OutputWindow::~OutputWindow(void)
{
	delete screenTri;

	GlobalConfig::RemovesListener("resolution", "outputwindow");
	GlobalConfig::RemoveParameter("resolution");
	GlobalConfig::RemoveParameter("srgb");

	glfwDestroyWindow(window);
	window = nullptr;
	glfwTerminate();
}	

void OutputWindow::ChangeWindowSize(const ei::IVec2& newResolution)
{
	glfwSetWindowSize(window, newResolution.x, newResolution.y);
	GL_CALL(glViewport, 0, 0, newResolution.x, newResolution.y);
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

void OutputWindow::DisplayHDRTexture(gl::Texture2D& texture, std::uint32_t _divisor)
{
	bool srgboutput = GlobalConfig::GetParameter("srgb")[0].As<bool>();
	if (srgboutput)
		glEnable(GL_FRAMEBUFFER_SRGB);


	texture.Bind(0);
	displayHDR.Activate();

	glUniform1ui(0, _divisor);
	screenTri->Draw();

	if (srgboutput)
		glDisable(GL_FRAMEBUFFER_SRGB);

    texture.ResetBinding(0);
}

void OutputWindow::Present()
{
	glfwSwapBuffers(window);
}


ei::IVec2 OutputWindow::GetResolution()
{
	ei::IVec2 resolution;
	glfwGetWindowSize(window, &resolution.x, &resolution.y);
	return resolution;
}
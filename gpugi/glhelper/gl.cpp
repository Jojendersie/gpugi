#include "gl.hpp"

#include "../utilities/assert.hpp"
#include "../utilities/logger.hpp"

namespace gl
{
	Result CheckGLError(const char* openGLFunctionName)
	{
		GLenum Error = glGetError();
		if (Error != GL_NO_ERROR)
		{
			const char* errorString;
			const char* description;
			switch (Error)
			{
			case GL_INVALID_ENUM:
				errorString = "GL_INVALID_ENUM";
				description = "An unacceptable value is specified for an enumerated argument. The offending command is ignored and has no other side effect than to set the error flag.";
				break;

			case GL_INVALID_VALUE:
				errorString = "GL_INVALID_VALUE";
				description = "A numeric argument is out of range. The offending command is ignored and has no other side effect than to set the error flag.";
				break;

			case GL_INVALID_OPERATION:
				errorString = "GL_INVALID_OPERATION";
				description = "The specified operation is not allowed in the current state. The offending command is ignored and has no other side effect than to set the error flag.";
				break;

			case GL_INVALID_FRAMEBUFFER_OPERATION:
				errorString = "GL_INVALID_FRAMEBUFFER_OPERATION";
				description = "The command is trying to render to or read from the framebuffer while the currently bound framebuffer is not framebuffer complete. "
					"The offending command is ignored and has no other side effect than to set the error flag.";
				break;

			case GL_OUT_OF_MEMORY:
				errorString = "GL_OUT_OF_MEMORY";
				description = "There is not enough memory left to execute the command. The state of the GL is undefined, except for the state of the error flags, after this error is recorded.";
				break;

			default:
				errorString = "UNKNOWN";
				description = "Unknown error code.";
				break;
			}
			LOG_ERROR(std::string("OpenGL Error during ") + openGLFunctionName + ": " + errorString + "(" + description + ")");
			return FAILURE;
		}

		return SUCCEEDED;
	}

	bool CheckGLFunctionExistsAndReport(const char* openGLFunctionName, const void* fkt)
	{
		if (fkt != nullptr)
			return true;
		else
		{
			LOG_ERROR(std::string("OpenGL operation ") + openGLFunctionName + " is not available, the function is nullptr!");
			return false;
		}
	};

	static void GLAPIENTRY GLDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
	{
		std::string debSource, debType, debSev;

		if (source == GL_DEBUG_SOURCE_API_ARB)
			debSource = "OpenGL";
		else if (source == GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB)
			debSource = "Windows";
		else if (source == GL_DEBUG_SOURCE_SHADER_COMPILER_ARB)
			debSource = "Shader Compiler";
		else if (source == GL_DEBUG_SOURCE_THIRD_PARTY_ARB)
			debSource = "Third Party";
		else if (source == GL_DEBUG_SOURCE_APPLICATION_ARB)
			debSource = "Application";
		else if (source == GL_DEBUG_SOURCE_OTHER_ARB)
			debSource = "Other";

		if (type == GL_DEBUG_TYPE_ERROR_ARB)
		{
			debType = "error";
		}
		else if (type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB)
		{
			debType = "deprecated behavior";
		}
		else if (type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB)
		{
			debType = "undefined behavior";
		}
		else if (type == GL_DEBUG_TYPE_PORTABILITY_ARB)
		{
			debType = "portability";
		}
		else if (type == GL_DEBUG_TYPE_PERFORMANCE_ARB)
		{
			debType = "performance";
		}
		else if (type == GL_DEBUG_TYPE_OTHER_ARB)
		{
			debType = "message";
		}

		if (severity == GL_DEBUG_SEVERITY_MEDIUM_ARB)
			debSev = "medium";
		else if (severity == GL_DEBUG_SEVERITY_LOW_ARB)
			debSev = "low";

		LOG_ERROR(debSource + ": " + debType + "(" + debSev + ") " + std::to_string(id) + ": " + message);
	}

	void ActivateGLDebugOutput(DebugSeverity level)
	{
		GL_CALL(glEnable, GL_DEBUG_OUTPUT);
		GL_CALL(glEnable, GL_DEBUG_OUTPUT_SYNCHRONOUS);
		switch (level)
		{
		case DebugSeverity::LOW:
			GL_CALL(glDebugMessageControl, GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, GL_TRUE);
		case DebugSeverity::MEDIUM:
			GL_CALL(glDebugMessageControl, GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
		case DebugSeverity::HIGH:
			GL_CALL(glDebugMessageControl, GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
		}

		GL_CALL(glDebugMessageCallback, &GLDebugOutput, nullptr);
	}
}
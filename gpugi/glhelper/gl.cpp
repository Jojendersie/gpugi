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

	static void GLAPIENTRY GLDebugOutput(GLenum _source, GLenum _type, GLuint _id, GLenum _severity, GLsizei _length, const GLchar* _message, const void* _userParam)
	{
		std::string debSource, debType, debSev;

		if (_source == GL_DEBUG_SOURCE_API)
			debSource = "OpenGL";
		else if (_source == GL_DEBUG_SOURCE_WINDOW_SYSTEM)
			debSource = "Windows";
		else if (_source == GL_DEBUG_SOURCE_SHADER_COMPILER)
			debSource = "Shader Compiler";
		else if (_source == GL_DEBUG_SOURCE_THIRD_PARTY)
			debSource = "Third Party";
		else if (_source == GL_DEBUG_SOURCE_APPLICATION)
			debSource = "Application";
		else if (_source == GL_DEBUG_SOURCE_OTHER)
			debSource = "Other";

		if (_type == GL_DEBUG_TYPE_ERROR)
		{
			debType = "error";
		}
		else if (_type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR)
		{
			debType = "deprecated behavior";
		}
		else if (_type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)
		{
			debType = "undefined behavior";
		}
		else if (_type == GL_DEBUG_TYPE_PORTABILITY)
		{
			debType = "portability";
		}
		else if (_type == GL_DEBUG_TYPE_PERFORMANCE)
		{
			debType = "performance";
		}
		else if (_type == GL_DEBUG_TYPE_OTHER)
		{
			debType = "message";
		}

		if (_severity == GL_DEBUG_SEVERITY_HIGH)
			debSev = "high";
		else if (_severity == GL_DEBUG_SEVERITY_MEDIUM)
			debSev = "medium";
		else if (_severity == GL_DEBUG_SEVERITY_LOW)
			debSev = "low";
		else if (_severity == GL_DEBUG_SEVERITY_NOTIFICATION)
			debSev = "note";
		
		std::string logMessage = debSource + ": " + debType + "(" + debSev + ") " + std::to_string(_id) + ": " + _message;
		if (_type == GL_DEBUG_TYPE_ERROR || _type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)
			LOG_ERROR(logMessage);
		else if (_type == GL_DEBUG_TYPE_PERFORMANCE)
			LOG_LVL1(logMessage);
		else
			LOG_LVL2(logMessage);
	}

	void ActivateGLDebugOutput(DebugSeverity level)
	{
		// Enabling
		switch (level)
		{
		case DebugSeverity::NOTIFICATION:
			GL_CALL(glDebugMessageControl, GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_TRUE);
		case DebugSeverity::LOW:
			GL_CALL(glDebugMessageControl, GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, GL_TRUE);
		case DebugSeverity::MEDIUM:
			GL_CALL(glDebugMessageControl, GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
		case DebugSeverity::HIGH:
			GL_CALL(glDebugMessageControl, GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
		}
		// Disabling
		switch (level)
		{
		case DebugSeverity::HIGH:
			GL_CALL(glDebugMessageControl, GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_FALSE);
		case DebugSeverity::MEDIUM:
			GL_CALL(glDebugMessageControl, GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, GL_FALSE);
		case DebugSeverity::LOW:
			GL_CALL(glDebugMessageControl, GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
		}

		GL_CALL(glEnable, GL_DEBUG_OUTPUT);
		GL_CALL(glEnable, GL_DEBUG_OUTPUT_SYNCHRONOUS);
		GL_CALL(glDebugMessageCallback, &GLDebugOutput, nullptr);
	}
}
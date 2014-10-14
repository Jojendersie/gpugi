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

}
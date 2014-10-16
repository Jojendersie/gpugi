#pragma once

#include "../utilities/utils.hpp"
#include <GL/glew.h>

namespace gl
{
	// Typedefs for different kinds of OpenGL IDs for better readability.
	typedef GLuint ShaderId;
	typedef GLuint ProgramId;

	typedef GLuint BufferId;
	typedef GLuint IndexBufferId;
	typedef GLuint VertexArrayObjectId;

	typedef GLuint TextureId;
	typedef GLuint Framebuffer;
	typedef GLuint SamplerId;

	typedef GLuint QueryId;


	// Error handling

	// Note that using GL_CALL may not be that important anymore if using the DebugMessage functionality (see ActivateDebugExtension)


	enum class DebugSeverity
	{
		LOW,
		MEDIUM,
		HIGH
	};

	/// Activates GL_DEBUG_OUTPUT.
	void ActivateGLDebugOutput(DebugSeverity level);

	/// Performs OpenGL error handling via glGetError and outputs results into the log.
	/// \param openGLFunctionName
	///		Name of the last OpenGL function that should is implicitly checked.
	Result CheckGLError(const char* openGLFunctionName);

	/// Checks weather the given function pointer is not null and reports an error if it does not exist.
	/// \returns false if fkt is nullptr.
	bool CheckGLFunctionExistsAndReport(const char* openGLFunctionName, const void* fkt);

#if _DEBUG

	// Internal macro that defines what happens on an OpenGL call.
	/// Macro for normal gl call.
#define __GL_CALL_HANDLING(openGLFunctionName, openGLFunction, ...) \
			{ \
		if (!CheckGLFunctionExistsAndReport(openGLFunctionName, openGLFunction)) \
			return FAILURE; \
		openGLFunction(__VA_ARGS__); \
		return CheckGLError(openGLFunctionName); \
			}

	/// Macro for gl call with return value.
#define __GL_CALL_HANDLING_RET(openGLFunctionName, openGLFunction, ...) \
			{ \
		if (!CheckGLFunctionExistsAndReport	(openGLFunctionName, openGLFunction)) \
			return 0; \
		auto out = openGLFunction(__VA_ARGS__); \
		CheckGLError(openGLFunctionName); \
		return out; \
			}

#else

	// Internal macro that defines what happens on an OpenGL call.
#define __GL_CALL_HANDLING(openGLFunctionName, openGLFunction, ...) \
			{ \
		openGLFunction(__VA_ARGS__); \
		return ::SUCCEEDED; \
			}

#define __GL_CALL_HANDLING_RET(openGLFunctionName, openGLFunction, ...) \
			{ \
		return openGLFunction(__VA_ARGS__); \
			}

#endif

	namespace Details
	{
		// GL call helper. The advantage of template function instead of a mere macro is that it can also be used in conditions!
		// The whole thing would be of course much easier with variadic templates. But MSVC <=120 lacking support forces us to do this.
		template<typename GlFunction, typename Arg0>
		Result CheckedGLCall(const char* openGLFunctionName, GlFunction openGLFunction, Arg0 arg0) __GL_CALL_HANDLING(openGLFunctionName, openGLFunction, arg0);
		template<typename GlFunction, typename Arg0, typename Arg1>
		Result CheckedGLCall(const char* openGLFunctionName, GlFunction openGLFunction, Arg0 arg0, Arg1 arg1) __GL_CALL_HANDLING(openGLFunctionName, openGLFunction, arg0, arg1);
		template<typename GlFunction, typename Arg0, typename Arg1, typename Arg2>
		Result CheckedGLCall(const char* openGLFunctionName, GlFunction openGLFunction, Arg0 arg0, Arg1 arg1, Arg2 arg2) __GL_CALL_HANDLING(openGLFunctionName, openGLFunction, arg0, arg1, arg2);
		template<typename GlFunction, typename Arg0, typename Arg1, typename Arg2, typename Arg3>
		Result CheckedGLCall(const char* openGLFunctionName, GlFunction openGLFunction, Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3) __GL_CALL_HANDLING(openGLFunctionName, openGLFunction, arg0, arg1, arg2, arg3);
		template<typename GlFunction, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
		Result CheckedGLCall(const char* openGLFunctionName, GlFunction openGLFunction, Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) __GL_CALL_HANDLING(openGLFunctionName, openGLFunction, arg0, arg1, arg2, arg3, arg4);
		template<typename GlFunction, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
		Result CheckedGLCall(const char* openGLFunctionName, GlFunction openGLFunction, Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5) __GL_CALL_HANDLING(openGLFunctionName, openGLFunction, arg0, arg1, arg2, arg3, arg4, arg5);
		template<typename GlFunction, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
		Result CheckedGLCall(const char* openGLFunctionName, GlFunction openGLFunction, Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6) __GL_CALL_HANDLING(openGLFunctionName, openGLFunction, arg0, arg1, arg2, arg3, arg4, arg5, arg6);
		template<typename GlFunction, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7>
		Result CheckedGLCall(const char* openGLFunctionName, GlFunction openGLFunction, Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7) __GL_CALL_HANDLING(openGLFunctionName, openGLFunction, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
		template<typename GlFunction, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8>
		Result CheckedGLCall(const char* openGLFunctionName, GlFunction openGLFunction, Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8) __GL_CALL_HANDLING(openGLFunctionName, openGLFunction, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
		template<typename GlFunction, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8, typename Arg9>
		Result CheckedGLCall(const char* openGLFunctionName, GlFunction openGLFunction, Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8, Arg9 arg9) __GL_CALL_HANDLING(openGLFunctionName, openGLFunction, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
		template<typename GlFunction, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8, typename Arg9, typename Arg10>
		Result CheckedGLCall(const char* openGLFunctionName, GlFunction openGLFunction, Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8, Arg9 arg9, Arg10 arg10) __GL_CALL_HANDLING(openGLFunctionName, openGLFunction, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);


		template<typename GlFunction>
		GLuint CheckedGLCall_Ret(const char* openGLFunctionName, GlFunction openGLFunction) __GL_CALL_HANDLING_RET(openGLFunctionName, openGLFunction);
		template<typename GlFunction, typename Arg0>
		GLuint CheckedGLCall_Ret(const char* openGLFunctionName, GlFunction openGLFunction, Arg0 arg0) __GL_CALL_HANDLING_RET(openGLFunctionName, openGLFunction, arg0);
		template<typename GlFunction, typename Arg0, typename Arg1>
		GLuint CheckedGLCall_Ret(const char* openGLFunctionName, GlFunction openGLFunction, Arg0 arg0, Arg1 arg1) __GL_CALL_HANDLING_RET(openGLFunctionName, openGLFunction, arg0, arg1);
		template<typename GlFunction, typename Arg0, typename Arg1, typename Arg2>
		GLuint CheckedGLCall_Ret(const char* openGLFunctionName, GlFunction openGLFunction, Arg0 arg0, Arg1 arg1, Arg2 arg2) __GL_CALL_HANDLING_RET(openGLFunctionName, openGLFunction, arg0, arg1, arg2);

	}

	// Remove internal call handling helper macro.
#undef __GL_CALL_HANDLING
#undef __GL_CALL_HANDLING_RET

	/// Recommend way to call any OpenGL function. Will perform optional nullptr and glGetError checks.
#define GL_CALL(OpenGLFunction, ...) \
	::gl::Details::CheckedGLCall(#OpenGLFunction, OpenGLFunction, __VA_ARGS__)

	/// There are a few functions that have a return value (glGet, glIsX, glCreateShader, glCreateProgram). Use this macro for those.
#define GL_RET_CALL(OpenGLFunction, ...) \
	::gl::Details::CheckedGLCall_Ret(#OpenGLFunction, OpenGLFunction, __VA_ARGS__)
}
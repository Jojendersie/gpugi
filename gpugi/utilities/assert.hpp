#pragma once

#include <string>

#ifdef DEBUG_BREAK
#undef DEBUG_BREAK
#endif

#if defined(_WIN32)
	#define DEBUG_BREAK do { __debugbreak(); } while(false)
#else
	#define DEBUG_BREAK do { ::kill(0, SIGTRAP); } while(false)
#endif

namespace Details
{
	/// Intern handler for assertions.
	void AssertHandler(const std::string& file, long line, const std::string& functionName,
						const std::string& conditionName, const std::string& errorMessage);
}

#ifdef _DEBUG
	#include <sstream>

	/// Default assert macro for all our needs. Use this instead of <cassert>
	#define Assert(condition, errorMessage) do { \
		if((condition) == false) \
		{   \
			std::stringstream str; \
			str << errorMessage; \
			::Details::AssertHandler(__FILE__, __LINE__, __FUNCTION__, #condition, str.str()); \
			DEBUG_BREAK; } \
		} while(false)
#else
	/// Default assert macro for all our needs. Use this instead of <cassert>
	#define Assert(condition, errorMessage) do { } while(false)
#endif
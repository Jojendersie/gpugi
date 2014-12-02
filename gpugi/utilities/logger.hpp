/// \brief Include this header in each file where you will using logging.
#pragma once

#ifndef LOG_LEVEL
#	error "You must define LOG_LEVEL {0,1,2} before including logger.hpp. Prefer to include logger at a central point to have only one define or set it in your project files."
#endif

#include "policy.hpp"
#include <mutex>
#include <sstream>

namespace Logger {

	/// \brief The logging facility which formats the input and writes
	///		it into some stream defined through a policy.
	///	\details The logging is thread safe
	class Logger
	{
	public:
    Logger();
		~Logger();
		
		/// \brief Create a logger which uses a certain policy for outputting data.
		/// \param [in] _policy A newly created instance of some class with a
		///		write function. The Logger takes the ownership and will delete
		///		the object itself.
		void Initialize( Policy* _policy );
		
    void Shutdown();

		/// \brief Format and write a log message.
		/// \details The first three arguments are induced by the macros and
		///		give the location of the call.
		/// \param [in] _message The message from the user
		void Write(const char* _file, const char* _function, long _codeLine, const char* _severity, const std::string& _message);

		/// \brief Format and write a log message.
		/// \details This variant does not show the file information and is
		///		used if LOG_NO_LOCALIZATION is defined.
		/// \param [in] _message The message from the user
		void Write(const char* _severity, const std::string& _message);

	private:
    bool m_initialized;
		Policy* m_policy;
		std::mutex m_mutex;

		std::string GetTimeString();
	};

	extern Logger g_logger;
} // namespace Logger


/// \brief Write an info/warning on level 0
#if (LOG_LEVEL == 0)
#	ifdef LOG_NO_LOCALIZATION
#		define LOG_LVL0(_message) do {\
			std::stringstream str; str << _message; \ 
Logger::g_logger.Write("Level 0 ", str.str()); } while(false)
#	else
#		define LOG_LVL0(_message) do {\
			std::stringstream str; str << _message; \
			Logger::g_logger.Write(__FILE__, __FUNCTION__, __LINE__, "Level 0 ", str.str()); } while(false)
#	endif
#else
#	define LOG_LVL0(...) do {} while(false)
#endif

/// \brief Write an info/warning if level 0 or 1 is active
#if (LOG_LEVEL <= 1)
#	ifdef LOG_NO_LOCALIZATION
#		define LOG_LVL1(_message) do {\
			std::stringstream str; str << _message; \
			Logger::g_logger.Write("Level 1 ", str.str()); } while(false)
#	else
#		define LOG_LVL1(_message) do {\
			std::stringstream str; str << _message; \
			Logger::g_logger.Write(__FILE__, __FUNCTION__, __LINE__, "Level 1 ", str.str()); } while(false)
#	endif
#else
#	define LOG_LVL1(...) do {} while(false)
#endif

/// \brief Write an info/warning if level 0, 1 or 2 is active
#if (LOG_LEVEL <= 2)
#	ifdef LOG_NO_LOCALIZATION
#		define LOG_LVL2(_message) do {\
			std::stringstream str; str << _message; \
			Logger::g_logger.Write("Level 2 ", str.str()); } while(false)
#	else
#		define LOG_LVL2(_message) do {\
			std::stringstream str; str << _message; \
		    Logger::g_logger.Write(__FILE__, __FUNCTION__, __LINE__, "Level 2 ", str.str()); } while(false)
#	endif
#else
#	define LOG_LVL2(...) do {} while(false)
#endif

/// \brief Write an error message - active on all levels.
#ifdef LOG_NO_LOCALIZATION
#	define LOG_ERROR(_message) do {\
			std::stringstream str; str << _message; \
			Logger::g_logger.Write("Error   ", str.str()); } while(false)
#else
#	define LOG_ERROR(_message) do {\
			std::stringstream str; str << _message; \
			Logger::g_logger.Write(__FILE__, __FUNCTION__, __LINE__, "Error   ", str.str()); } while(false)
#endif

/// \brief Write an error message and than throw an exception with the
///		message string.
#ifdef LOG_NO_LOCALIZATION
#	define LOG_CRITICAL(_message) do {\
			std::stringstream str; str << _message; \
			{Logger::g_logger.Write("Critical", str.str()); throw (str.str());} while(false)
#else
#	define LOG_CRITICAL(_message) do {\
			std::stringstream str; str << _message; \
			Logger::g_logger.Write(__FILE__, __FUNCTION__, __LINE__, "Critical", str.str()); throw (str.str());} while(false)
#endif
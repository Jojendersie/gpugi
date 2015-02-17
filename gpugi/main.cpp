
#include "application.hpp"
#include "utilities/logger.hpp"

#ifdef _WIN32
	#undef APIENTRY
	#define NOMINMAX
	#include <windows.h>	
#endif

#include <iostream>


#ifdef _CONSOLE
// Console window handler
BOOL CtrlHandler(DWORD fdwCtrlType)
{
	if (fdwCtrlType == CTRL_CLOSE_EVENT)
	{
		Logger::g_logger.Shutdown(); // To avoid crash with invalid mutex.
		return FALSE;
	}
	else
		return FALSE;
}
#endif

int main(int argc, char** argv)
{
	// Install console handler.
#ifdef _CONSOLE
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
#endif

	// Actual application.
	try
	{
		Application application(argc, argv);
		application.Run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Unhandled exception:\n";
		std::cerr << "Message: " << e.what() << std::endl;
		__debugbreak();
		return 1;
	}
	catch (std::string& str)
	{
		std::cerr << "Unhandled exception:\n";
		std::cerr << "Message: " << str << std::endl;
		__debugbreak();
		return 1;
	}
	catch (...)
	{
		std::cerr << "Unknown exception!" << std::endl;
		__debugbreak();
		return 1;
	}
	return 0;
}
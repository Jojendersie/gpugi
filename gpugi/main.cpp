#include <iostream>
#include <memory>
#include "outputwindow.hpp"
#include "Time/Stopwatch.h"

#include "utilities/loggerinit.hpp"

#include "renderer/testrenderer.hpp"

const unsigned int defaultWindowWidth = 1024;
const unsigned int defaultWindowHeight = 768;

class Application
{
public:
	Application()
	{
	}

	void Init()
	{
		std::cout << "Init window ...\n";
		window.reset(new OutputWindow(defaultWindowWidth, defaultWindowHeight));

		renderer.reset(new TestRenderer());
	}

	~Application()
	{
		SaveImage();
	}

	void Run()
	{
		// Main loop
		ezStopwatch mainLoopStopWatch;
		while(window->IsWindowAlive())
		{
			ezTime mainLoopTime = mainLoopStopWatch.GetRunningTotal();
			mainLoopStopWatch.StopAndReset();
			mainLoopStopWatch.Resume();

			window->PollWindowEvents();

			Draw();
			Input();
		}
	}

private:

	void Draw()
	{
		renderer->Draw();

		window->DisplayHDRTexture(renderer->GetBackbuffer());
		window->Present();
	}

	void Input()
	{
		// Save image on F10/PrintScreen
		if (window->WasButtonPressed(GLFW_KEY_PRINT_SCREEN) || window->WasButtonPressed(GLFW_KEY_F10))
			SaveImage();

		// Add more useful hotkeys on demand!
	}

	void SaveImage()
	{
		// TODO: Save a HDR image to a default location with a clever name
	}

	std::unique_ptr<Renderer> renderer;
	std::unique_ptr<OutputWindow> window;
};

int main(int argc, char** argv)
{
	// Logger init.
	Logger::FilePolicy* filePolicy = new Logger::FilePolicy("log.txt");
	Logger::g_logger.Initialize(filePolicy);


	// Actual application.
	try
	{
		Application application;
		application.Init();
		application.Run();
	}
	catch(std::exception e)
	{
		std::cerr << "Unhandled exception:\n";
		std::cerr << "Message: " << e.what() << std::endl;
		__debugbreak();
		return 1;
	}
	catch(std::string str)
	{
		std::cerr << "Unhandled exception:\n";
		std::cerr << "Message: " << str << std::endl;
		__debugbreak();
		return 1;
	}
	catch(...)
	{
		std::cerr << "Unknown exception!" << std::endl;
		__debugbreak();
		return 1;
	}

	// FIXME/TODO: Deleting the logger policy is not possible since the logger is destructed later -.-

	return 0;
}
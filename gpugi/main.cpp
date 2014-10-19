#include <iostream>
#include <memory>
#include "outputwindow.hpp"
#include "Time/Stopwatch.h"

#include "utilities/loggerinit.hpp"

#include "control/globalconfig.hpp"
#include "control/scriptprocessing.hpp"

#include "renderer/testrenderer.hpp"

class Application
{
public:
	Application(int argc, char** argv)
	{
		// Logger init.
		Logger::FilePolicy* filePolicy = new Logger::FilePolicy("log.txt");
		Logger::g_logger.Initialize(filePolicy);

		// Add a few fundamental global parameters/events.
		GlobalConfig::AddParameter("pause", { 0.0f }, "Set to <=0 to pause drawing. Input will still work.");
		GlobalConfig::AddParameter("resolution", { 1024, 768 }, "The window's width and height.");
		GlobalConfig::AddParameter("help", { }, "Dumps all available parameter/events to the command window.");
		GlobalConfig::AddListener("help", "HelpDumpFunc", [](const GlobalConfig::ParameterType) { std::cout << GlobalConfig::GetEntryDescriptions() << std::endl; });

		// Window...
		LOG_LVL2("Init window ...");
		window.reset(new OutputWindow());

		// Renderer...
		renderer.reset(new TestRenderer());

		// Init console input.
		ScriptProcessing::StartCommandWindowThread();

		// Load command script if there's a parameter
		if (argc > 1)
			ScriptProcessing::RunScript(argv[1]);
	}

	~Application()
	{
		SaveImage();
		ScriptProcessing::StopCommandWindowThread();
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
			ScriptProcessing::ProcessCommandQueue();

			if (GlobalConfig::GetParameter("pause")[0] <= 0.0f)
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
	// Actual application.
	try
	{
		Application application(argc, argv);
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
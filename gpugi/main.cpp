#include <iostream>
#include <memory>
#include "outputwindow.hpp"
#include "Time/Stopwatch.h"

#include "utilities/loggerinit.hpp"

#include "control/globalconfig.hpp"
#include "control/scriptprocessing.hpp"

#include "renderer/testrenderer.hpp"

#include "glhelper/texture2d.hpp"

#include "hdrimage.hpp"

#include <ctime>


class Application
{
public:
	Application(int argc, char** argv) : screenShotName("screenshot")
	{
		// Logger init.
		Logger::FilePolicy* filePolicy = new Logger::FilePolicy("log.txt");
		Logger::g_logger.Initialize(filePolicy);

		// Add a few fundamental global parameters/events.
		GlobalConfig::AddParameter("pause", { 0.0f }, "Set to <=0 to pause drawing. Input will still work.");
		GlobalConfig::AddParameter("resolution", { 1024, 768 }, "The window's width and height.");
		GlobalConfig::AddParameter("help", { }, "Dumps all available parameter/events to the command window.");
		GlobalConfig::AddListener("help", "HelpDumpFunc", [](const GlobalConfig::ParameterType) { std::cout << GlobalConfig::GetEntryDescriptions() << std::endl; });
		GlobalConfig::AddParameter("screenshot", {}, "Saves a screenshot.");
		GlobalConfig::AddListener("screenshot", "SaveScreenshot", [=](const GlobalConfig::ParameterType) { this->SaveImage(); });


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

	std::string UIntToMinLengthString(int number, int minDigits)
	{
		int zeros = std::max(0, minDigits - static_cast<int>(ceil(log10(number))));
		std::string out = std::string("0", zeros) + std::to_string(number);
		return out;
	}

	void SaveImage()
	{
		time_t t = time(0);   // get time now
		struct tm* now = localtime(&t);
		std::string filename = screenShotName + " " + UIntToMinLengthString(now->tm_mday, 2) + "." + UIntToMinLengthString(now->tm_mon, 2) + " " +
								UIntToMinLengthString(now->tm_hour, 2) + "h" + UIntToMinLengthString(now->tm_min, 2) + "m" + std::to_string(now->tm_sec) + "s.pfm";
		gl::Texture2D& backbuffer = renderer->GetBackbuffer();
		std::unique_ptr<ei::Vec4[]> imageData(new ei::Vec4[backbuffer.GetWidth() * backbuffer.GetHeight()]);
		backbuffer.ReadImage(0, gl::TextureReadFormat::RGBA, gl::TextureReadType::FLOAT, backbuffer.GetWidth() * backbuffer.GetHeight() * sizeof(ei::Vec4), imageData.get());
		WritePfm(imageData.get(), ei::IVec2(backbuffer.GetWidth(), backbuffer.GetHeight()), filename);
		LOG_LVL1("Wrote screenshot \"" + filename + "\"");
	}

	std::string screenShotName;
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
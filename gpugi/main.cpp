#include <iostream>
#include <memory>
#include "outputwindow.hpp"
#include "Time/Stopwatch.h"

#include "utilities/loggerinit.hpp"

#include "control/globalconfig.hpp"
#include "control/scriptprocessing.hpp"

#include "renderer/testrenderer.hpp"

#include "camera/interactivecamera.hpp"

#include "glhelper/texture2d.hpp"

#include "hdrimage.hpp"

#include <ctime>

#ifdef _WIN32
	#undef APIENTRY
	#define NOMINMAX
	#include <windows.h>
#endif


class Application
{
public:
	Application(int argc, char** argv) : screenShotName("screenshot")
	{
		// Logger init.
		Logger::g_logger.Initialize(new Logger::FilePolicy("log.txt"));

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

		// Create "global" camera.
		camera.reset(new InteractiveCamera(window->GetGLFWWindow(), ei::Vec3(0.0f), ei::Vec3(0.0f, 0.0f, 1.0f), 
											GlobalConfig::GetParameter("resolution")[0] / GlobalConfig::GetParameter("resolution")[1], 70.0f));
		camera->ConnectToGlobalConfig();

		// Renderer...
		LOG_LVL2("Init window ...");
		renderer.reset(new TestRenderer(*camera));

		// Load command script if there's a parameter
		if (argc > 1)
			ScriptProcessing::RunScript(argv[1]);

		// Init console input.
		ScriptProcessing::StartCommandWindowThread();
	}

	~Application()
	{
		SaveImage();

		// Only known method to kill the console.
#ifdef _WIN32
		FreeConsole();
#endif

		ScriptProcessing::StopCommandWindowThread();
	}

	void Run()
	{
		// Main loop
		ezStopwatch mainLoopStopWatch;
		while(window->IsWindowAlive())
		{
			ezTime timeSinceLastUpdate = mainLoopStopWatch.GetRunningTotal();
			mainLoopStopWatch.StopAndReset();
			mainLoopStopWatch.Resume();

			Update(timeSinceLastUpdate);
			if (GlobalConfig::GetParameter("pause")[0] <= 0.0f)
				Draw();
		}
	}

private:

	void Update(ezTime timeSinceLastUpdate)
	{
		window->PollWindowEvents();
		ScriptProcessing::ProcessCommandQueue();

		Input();

		if (camera->Update(timeSinceLastUpdate))
			renderer->SetCamera(*camera);
	}

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
	std::unique_ptr<InteractiveCamera> camera;
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

	return 0;
}
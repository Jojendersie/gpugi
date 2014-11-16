#include <iostream>
#include <memory>
#include "outputwindow.hpp"
#include "Time/Stopwatch.h"

#include "utilities/loggerinit.hpp"

#include "control/globalconfig.hpp"
#include "control/scriptprocessing.hpp"

#include "renderer/testrenderer.hpp"
#include "renderer/referencerenderer.hpp"
#include "renderer/lightpathtracer.hpp"
//#include "renderer/debugrenderer.hpp"

#include "camera/interactivecamera.hpp"

#include "scene/scene.hpp"

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
	Application(int argc, char** argv) : m_screenShotName("screenshot")
	{
		// Logger init.
		Logger::g_logger.Initialize(new Logger::FilePolicy("log.txt"));

		// Add a few fundamental global parameters/events.
		GlobalConfig::AddParameter("pause", { false }, "Set to true to pause drawing. Input will still work.");
		GlobalConfig::AddParameter("resolution", { 1024, 768 }, "The window's width and height.");
		GlobalConfig::AddParameter("help", {}, "Dumps all available parameter/events to the command window.");
		GlobalConfig::AddListener("help", "HelpDumpFunc", [](const GlobalConfig::ParameterType&) { std::cout << GlobalConfig::GetEntryDescriptions() << std::endl; });
		GlobalConfig::AddParameter("screenshot", {}, "Saves a screenshot.");
		GlobalConfig::AddListener("screenshot", "SaveScreenshot", [=](const GlobalConfig::ParameterType) { this->SaveImage(); });

		// Window...
		LOG_LVL2("Init window ...");
		m_window.reset(new OutputWindow());

		// Create "global" camera.
		m_camera.reset(new InteractiveCamera(m_window->GetGLFWWindow(), ei::Vec3(0.0f), ei::Vec3(0.0f, 0.0f, 1.0f), 
							GlobalConfig::GetParameter("resolution")[0].As<float>() / GlobalConfig::GetParameter("resolution")[1].As<int>(), 70.0f));
		m_camera->ConnectToGlobalConfig();
		GlobalConfig::AddListener("resolution", "global camera aspect", [=](const GlobalConfig::ParameterType& p) {
			m_camera->SetAspectRatio(p[0].As<float>() / p[1].As<float>());
			m_renderer->SetCamera(*m_camera);
		});
		auto updateCamera = [=](const GlobalConfig::ParameterType& p){
			this->m_renderer->SetCamera(*m_camera);
		};
		GlobalConfig::AddListener("cameraPos", "update renderer cam", updateCamera);
		GlobalConfig::AddListener("cameraLookAt", "update renderer cam", updateCamera);
		GlobalConfig::AddListener("cameraFOV", "update renderer cam", updateCamera);

		// Renderer...
		LOG_LVL2("Init renderer ...");
		m_renderer.reset(new LightPathTracer(*m_camera));//ReferenceRenderer(*m_camera));
		GlobalConfig::AddParameter("sceneFilename", { std::string("") }, "Change this value to load a new scene.");
		GlobalConfig::AddListener("sceneFilename", "LoadScene", [=](const GlobalConfig::ParameterType p) {
			std::string sceneFilename = p[0].As<std::string>();
			std::cout << "Loading scene " << sceneFilename;
			m_scene = std::make_shared<Scene>(sceneFilename);
			m_renderer->SetScene(m_scene);
		});

		// Load command script if there's a parameter
		if (argc > 1)
			m_scriptProcessing.RunScript(argv[1]);

		// Init console input.
		m_scriptProcessing.StartConsoleWindowThread();
	}

	~Application()
	{
		SaveImage();

		// Only known method to kill the console.
#ifdef _WIN32
		FreeConsole();
#endif

		m_scriptProcessing.StopConsoleWindowThread();
	}

	void Run()
	{
		// Main loop
		ezStopwatch mainLoopStopWatch;
		while(m_window->IsWindowAlive())
		{
			ezTime timeSinceLastUpdate = mainLoopStopWatch.GetRunningTotal();
			mainLoopStopWatch.StopAndReset();
			mainLoopStopWatch.Resume();

			Update(timeSinceLastUpdate);
			if (!GlobalConfig::GetParameter("pause")[0].As<bool>())
				Draw();
		}
	}

private:

	void Update(ezTime timeSinceLastUpdate)
	{
		m_window->PollWindowEvents();
		m_scriptProcessing.ProcessCommandQueue(timeSinceLastUpdate.GetSeconds());

		Input();

		if (m_camera->Update(timeSinceLastUpdate))
			m_renderer->SetCamera(*m_camera);


		m_window->SetTitle("iteration: " + std::to_string(m_renderer->GetIterationCount()) + " - time per frame " + 
							std::to_string(timeSinceLastUpdate.GetMilliseconds()) + "ms (FPS: " + std::to_string(1.0f / timeSinceLastUpdate.GetSeconds()));
	}

	void Draw()
	{
		m_renderer->Draw();

		m_window->DisplayHDRTexture(m_renderer->GetBackbuffer(), m_renderer->GetIterationCount());
		m_window->Present();
	}

	void Input()
	{
		// Save image on F10/PrintScreen
		if (m_window->WasButtonPressed(GLFW_KEY_PRINT_SCREEN) || m_window->WasButtonPressed(GLFW_KEY_F10))
			SaveImage();

		// Add more useful hotkeys on demand!
	}

	std::string UIntToMinLengthString(int _number, int _minDigits)
	{
		int zeros = std::max(0, _minDigits - static_cast<int>(ceil(log10(_number+1))));
		std::string out = std::string(zeros, '0') + std::to_string(_number);
		return out;
	}

	void SaveImage()
	{
		time_t t = time(0);   // get time now
		struct tm* now = localtime(&t);
		std::string filename = m_screenShotName + " " + UIntToMinLengthString(now->tm_mon+1, 2) + "." + UIntToMinLengthString(now->tm_mday, 2) + " " +
								UIntToMinLengthString(now->tm_hour, 2) + "h" + UIntToMinLengthString(now->tm_min, 2) + "m" + std::to_string(now->tm_sec) + "s.pfm";
		gl::Texture2D& backbuffer = m_renderer->GetBackbuffer();
		std::unique_ptr<ei::Vec4[]> imageData(new ei::Vec4[backbuffer.GetWidth() * backbuffer.GetHeight()]);
		backbuffer.ReadImage(0, gl::TextureReadFormat::RGBA, gl::TextureReadType::FLOAT, backbuffer.GetWidth() * backbuffer.GetHeight() * sizeof(ei::Vec4), imageData.get());
		WritePfm(imageData.get(), ei::IVec2(backbuffer.GetWidth(), backbuffer.GetHeight()), filename, m_renderer->GetIterationCount());
		LOG_LVL1("Wrote screenshot \"" + filename + "\"");
	}

	ScriptProcessing m_scriptProcessing;
	std::string m_screenShotName;
	std::unique_ptr<Renderer> m_renderer;
	std::unique_ptr<OutputWindow> m_window;
	std::unique_ptr<InteractiveCamera> m_camera;
    std::shared_ptr<Scene> m_scene;
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
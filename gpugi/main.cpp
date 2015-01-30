#include <iostream>
#include <memory>
#include "outputwindow.hpp"
#include "Time/Stopwatch.h"

#include "utilities/loggerinit.hpp"

#include "control/globalconfig.hpp"
#include "control/scriptprocessing.hpp"

#include "renderer/pathtracer.hpp"
#include "renderer/lightpathtracer.hpp"
#include "renderer/bidirectionalpathtracer.hpp"

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
	Application(int argc, char** argv) : m_shutdown(false)
	{
		// Logger init.
		Logger::g_logger.Initialize(new Logger::FilePolicy("log.txt"));

		// Window...
		LOG_LVL2("Init window ...");
		m_window.reset(new OutputWindow());

		// Create "global" camera.
		m_camera.reset(new InteractiveCamera(m_window->GetGLFWWindow(), ei::Vec3(0.0f), ei::Vec3(0.0f, 0.0f, 1.0f), 
											static_cast<float>(m_window->GetResolution().x) / m_window->GetResolution().y, 70.0f));

		// Register various script commands.
		RegisterScriptCommands();

		// Load command script if there's a parameter
		if (argc > 1)
			m_scriptProcessing.RunScript(argv[1]);

		// Init console input.
		m_scriptProcessing.StartConsoleWindowThread();
	}

	void RegisterScriptCommands()
	{
		// Add a few fundamental global parameters/events.
		GlobalConfig::AddParameter("pause", { false }, "Set to true to pause drawing. Input will still work.");
		GlobalConfig::AddParameter("help", {}, "Dumps all available parameter/events to the command window.");
		GlobalConfig::AddListener("help", "HelpDumpFunc", [](const GlobalConfig::ParameterType&) { std::cout << GlobalConfig::GetEntryDescriptions() << std::endl; });
		GlobalConfig::AddParameter("exit", {}, "Exits the application. Does NOT take a screenshot.");
		GlobalConfig::AddListener("exit", "exit application", [&](const GlobalConfig::ParameterType&) { m_shutdown = true; });
		GlobalConfig::AddParameter("tic", {}, "Resets stopwatch.");
		GlobalConfig::AddListener("tic", "stopwatch reset", [&](const GlobalConfig::ParameterType&) { m_stopwatch.StopAndReset(); m_stopwatch.Resume(); });
		GlobalConfig::AddParameter("toc", {}, "Gets current stopwatch time.");
		GlobalConfig::AddListener("toc", "stopwatch reset", [&](const GlobalConfig::ParameterType&) { LOG_LVL2(m_stopwatch.GetRunningTotal().GetSeconds() << "s"); });

		// Screenshots
		GlobalConfig::AddParameter("screenshot", {}, "Saves a screenshot.");
		GlobalConfig::AddListener("screenshot", "SaveScreenshot", [=](const GlobalConfig::ParameterType&) { this->SaveImage(); });
		GlobalConfig::AddParameter("namedscreenshot", { std::string("") }, "Saves a screenshot with the given name.");
		GlobalConfig::AddListener("namedscreenshot", "SaveScreenshot", [=](const GlobalConfig::ParameterType& p) { this->SaveImage(p[0].As<std::string>()); });

		// Camera
		m_camera->ConnectToGlobalConfig();
		auto updateCamera = [=](const GlobalConfig::ParameterType& p){
			if (m_renderer)
				m_renderer->SetCamera(*m_camera);
		};
		GlobalConfig::AddListener("cameraPos", "update renderer cam", updateCamera);
		GlobalConfig::AddListener("cameraLookAt", "update renderer cam", updateCamera);
		GlobalConfig::AddListener("cameraFOV", "update renderer cam", updateCamera);


		// Renderer change function.
		GlobalConfig::AddParameter("renderer", { 0 }, "Change this value to change the active renderer (resets previous results!).\n"
			"0: Pathtracer (\"ReferenceRenderer\"\n"
			"1: LightPathtracer\n"
			"2: BidirectionalPathtracer");
		GlobalConfig::AddListener("renderer", "ChangeRenderer", std::bind(&Application::SwitchRenderer, this, std::placeholders::_1));

		// Resolution change
		GlobalConfig::AddListener("resolution", "global camera aspect", [=](const GlobalConfig::ParameterType& p) {
			m_camera->SetAspectRatio(p[0].As<float>() / p[1].As<float>());
			if (m_renderer)
			{
				m_renderer->SetCamera(*m_camera);
				m_renderer->SetScreenSize(ei::IVec2(p[0].As<int>(), p[1].As<int>()));
			}
		});

		// Scene change functions.
		GlobalConfig::AddParameter("sceneFilename", { std::string("") }, "Change this value to load a new scene.");
		GlobalConfig::AddListener("sceneFilename", "LoadScene", [=](const GlobalConfig::ParameterType& p) {
			std::string sceneFilename = p[0].As<std::string>();
			std::cout << "Loading scene " << sceneFilename;
			m_scene = std::make_shared<Scene>(sceneFilename);
			if (m_renderer)
				m_renderer->SetScene(m_scene);
		});
	}
	

	~Application()
	{
		if (!m_shutdown)
			SaveImage();

		// Only known method to kill the console.
#ifdef _WIN32
		FreeConsole();
#endif

		m_scriptProcessing.StopConsoleWindowThread();

		Logger::g_logger.Shutdown();
	}

	void SwitchRenderer(const GlobalConfig::ParameterType& p)
	{
		glFlush();
		switch (p[0].As<int>())
		{
		case 0:
			LOG_LVL2("Switching to Pathtracer...");
			m_renderer.reset();
			m_renderer.reset(new Pathtracer());
			break;

		case 1:
			LOG_LVL2("Switching to LightPathtracer...");
			m_renderer.reset();
			m_renderer.reset(new LightPathtracer());
			break;

		case 2:
			LOG_LVL2("Switching to Bidirectional Pathtracer...");
			m_renderer.reset();
			m_renderer.reset(new BidirectionalPathtracer());
			break;

		default:
			LOG_ERROR("Unknown renderer index!");
			return;
		}

		m_renderer->SetScreenSize(m_window->GetResolution());
		m_renderer->SetCamera(*m_camera);
		if (m_scene)
		{
			m_renderer->SetScene(m_scene);
		}
		m_renderer->RegisterDebugRenderStateConfigOptions();
	}

	void Run()
	{
		// Main loop
		ezStopwatch mainLoopStopWatch;
		while(m_window->IsWindowAlive() && !m_shutdown)
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

		if (m_renderer)
		{
			if (m_camera->Update(timeSinceLastUpdate))
				m_renderer->SetCamera(*m_camera);


			m_window->SetTitle(m_renderer->GetName() + " - iteration: " + std::to_string(m_renderer->GetIterationCount()) + " - time per frame " +
				std::to_string(timeSinceLastUpdate.GetMilliseconds()) + "ms (FPS: " + std::to_string(1.0f / timeSinceLastUpdate.GetSeconds()) + ")");
		}
		else
			m_window->SetTitle("No renderer!");
	}

	void Draw()
	{
		if (!m_renderer)
			return;

		m_renderer->Draw();

		if (!m_renderer->IsDebugRendererActive())
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

	void SaveImage(const std::string& name = "")
	{
		if (!m_renderer)
			return;

		time_t t = time(0);   // get time now
		struct tm* now = localtime(&t);
        std::string date = UIntToMinLengthString(now->tm_mon+1, 2) + "." + UIntToMinLengthString(now->tm_mday, 2) + " " +
						   UIntToMinLengthString(now->tm_hour, 2) + "h" + UIntToMinLengthString(now->tm_min, 2) + "m" + std::to_string(now->tm_sec) + "s ";
		std::string filename = date + m_renderer->GetName() + " " +
                                 std::to_string(m_renderer->GetIterationCount()) + "it"
                                ".pfm";
		if (!name.empty())
			filename = name + " " + filename;
		gl::Texture2D& backbuffer = m_renderer->GetBackbuffer();
		std::unique_ptr<ei::Vec4[]> imageData(new ei::Vec4[backbuffer.GetWidth() * backbuffer.GetHeight()]);
		backbuffer.ReadImage(0, gl::TextureReadFormat::RGBA, gl::TextureReadType::FLOAT, backbuffer.GetWidth() * backbuffer.GetHeight() * sizeof(ei::Vec4), imageData.get());
		if(WritePfm(imageData.get(), ei::IVec2(backbuffer.GetWidth(), backbuffer.GetHeight()), filename, m_renderer->GetIterationCount()))
			LOG_LVL1("Wrote screenshot \"" + filename + "\"");
	}

	ScriptProcessing m_scriptProcessing;
	std::unique_ptr<Renderer> m_renderer;
	std::unique_ptr<OutputWindow> m_window;
	std::unique_ptr<InteractiveCamera> m_camera;
    std::shared_ptr<Scene> m_scene;

	ezStopwatch m_stopwatch;
	bool m_shutdown;
};


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
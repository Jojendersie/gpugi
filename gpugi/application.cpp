#include "application.hpp"

#include <iostream>
#include "outputwindow.hpp"

#include "utilities/loggerinit.hpp"

#include "control/scriptprocessing.hpp"

#include "renderer/renderersystem.hpp"
#include "renderer/pathtracer.hpp"
#include "renderer/whittedraytracer.hpp"
#include "renderer/lightpathtracer.hpp"
#include "renderer/pixelcachelighttracer.hpp"
#include "renderer/bidirectionalpathtracer.hpp"
#include "renderer/hierarchyimportance.hpp"
#include "renderer/debugrenderer/hierarchyvisualization.hpp"
#include "renderer/debugrenderer/raytracemeshinfo.hpp"

#include "camera/interactivecamera.hpp"

#include "scene/scene.hpp"

#include "glhelper/texture2d.hpp"

#include "imageut/hdrimage.hpp"
#include "imageut/texturemse.hpp"

#include <ctime>
#include <limits>

#ifdef _WIN32
	#undef APIENTRY
	#define NOMINMAX
	#include <windows.h>	
#endif

Application::Application(int argc, char** argv) : m_shutdown(false),
		m_iterationSinceLastMSECheck(0), m_scriptWaitsForMSE(false)
{
	// Logger init.
	Logger::g_logger.Initialize(new Logger::FilePolicy("log.txt"));

	// Window...
	LOG_LVL2("Init window ...");
	m_window = std::make_unique<OutputWindow>();

	// Create "global" camera.
	m_camera = std::make_unique<InteractiveCamera>(m_window->GetGLFWWindow(), ei::Vec3(0.0f), ei::Vec3(0.0f, 0.0f, 1.0f),
													static_cast<float>(m_window->GetResolution().x) / m_window->GetResolution().y, 70.0f);

	// Create persistent render system.
	m_rendererSystem = std::make_unique<RendererSystem>();

	// Module for image-MSE checks.
	m_textureMSE = std::make_unique<TextureMSE>();

	// Register various script commands.
	RegisterScriptCommands();

	// Load command script if there's a parameter
	if (argc > 1)
		m_scriptProcessing.RunScript(argv[1]);

	// Init console input.
	m_scriptProcessing.StartConsoleWindowThread();
}

void Application::RegisterScriptCommands()
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
	auto updateCamera = [=](const GlobalConfig::ParameterType& p) { m_rendererSystem->SetCamera(*m_camera); };
	GlobalConfig::AddListener("cameraPos", "update renderer cam", updateCamera);
	GlobalConfig::AddListener("cameraLookAt", "update renderer cam", updateCamera);
	GlobalConfig::AddListener("cameraFOV", "update renderer cam", updateCamera);


	// Renderer change function.
	GlobalConfig::AddParameter("renderer", { 0 }, "Change this value to change the active renderer (resets previous results!).\n"
													"0: Pathtracer\n"
													"1: LightPathtracer\n"
													"2: BidirectionalPathtracer\n"
													"3: HierarchyImportance\n"
													"4: Whitted Pathtracer\n"
													"5: Pixel Cachc Lighttracer");
	GlobalConfig::AddListener("renderer", "ChangeRenderer", std::bind(&Application::SwitchRenderer, this, std::placeholders::_1));

	GlobalConfig::AddParameter("debugrenderer", { -1 }, "Set active debug renderer (will not change the current renderer).\n"
														"-1: to disable any debug rendering.\n"
														"0: RayTraceMeshInfo\n"
														"1: Hierachyvisualization");
	GlobalConfig::AddListener("debugrenderer", "Renderer", std::bind(&Application::SwitchDebugRenderer, this, std::placeholders::_1));


	// Resolution change
	GlobalConfig::AddListener("resolution", "global camera aspect", [=](const GlobalConfig::ParameterType& p) {
		m_camera->SetAspectRatio(p[0].As<float>() / p[1].As<float>());
		m_rendererSystem->SetCamera(*m_camera);
		m_rendererSystem->SetScreenSize(ei::IVec2(p[0].As<int>(), p[1].As<int>()));
	});

	// Scene change functions.
	GlobalConfig::AddParameter("sceneFilename", { std::string("") }, "Change this value to load a new scene.");
	GlobalConfig::AddListener("sceneFilename", "LoadScene", [=](const GlobalConfig::ParameterType& p) {
		std::string sceneFilename = p[0].As<std::string>();
		std::cout << "Loading scene " << sceneFilename;
		m_scene = std::make_shared<Scene>(sceneFilename);
		m_rendererSystem->SetScene(m_scene);

		InteractiveCamera* icam = dynamic_cast<InteractiveCamera*>(m_camera.get());
		if (icam)
			icam->SetMoveSpeed(max(m_scene->GetBoundingBox().max - m_scene->GetBoundingBox().min) / 15.0f);
	});

	// MSE check
	GlobalConfig::AddParameter("referenceImage", { std::string("") }, "Reference for all mse checks.");
	GlobalConfig::AddListener("referenceImage", "LoadReference", [=](const GlobalConfig::ParameterType& p) {
		m_textureMSE->LoadReferenceFromPFM(p[0].As<std::string>());
	});
	GlobalConfig::AddParameter("mseCheckInterval", { 0 }, "MSE will be computed every xth iteration. 0 for no MSE computation");
	GlobalConfig::AddParameter("waitMSE", { 0 }, "Script execution halts until the MSE is lower than the given value. Only used if mseCheckInterval is >0.");
	GlobalConfig::AddListener("waitMSE", "ScriptWait", [&](const GlobalConfig::ParameterType& p) {
		if (GlobalConfig::GetParameter("mseCheckInterval")[0].As<int>() <= 0)
			LOG_ERROR("Can't activate waitMSE as long the mseCheckInterval is 0!");
		else if (!m_textureMSE->HasValidReferenceImage())
			LOG_ERROR("Can't activate waitMSE as long as there is no reference image loaded!");
		else
			m_scriptWaitsForMSE = true;
	});
	m_scriptProcessing.AddProcessingPauseCommand("waitMSE");
}
	

Application::~Application()
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

void Application::SwitchRenderer(const GlobalConfig::ParameterType& p)
{
	glFlush();
	switch (p[0].As<int>())
	{
	case 0:
		LOG_LVL2("Switching to Pathtracer...");
		m_rendererSystem->SetRenderer<Pathtracer>();
		break;

	case 1:
		LOG_LVL2("Switching to LightPathtracer...");
		m_rendererSystem->SetRenderer<LightPathtracer>();
		break;

	case 2:
		LOG_LVL2("Switching to Bidirectional Pathtracer...");
		m_rendererSystem->SetRenderer<BidirectionalPathtracer>();
		break;

	case 3:
		LOG_LVL2("Switching to Hierarchy Importance Renderer...");
		m_rendererSystem->SetRenderer<HierarchyImportance>();
		break;

	case 4:
		LOG_LVL2("Switching to Whitted Raytracer...");
		m_rendererSystem->SetRenderer<WhittedRayTracer>();
		break;

	case 5:
		LOG_LVL2("Switching to Pixelcache Lighttracing...");
		m_rendererSystem->SetRenderer<PixelCacheLighttracer>();
		break;

	default:
		LOG_ERROR("Unknown renderer index!");
		return;
	}
}

void Application::SwitchDebugRenderer(const GlobalConfig::ParameterType& p)
{
	switch (p[0].As<int>())
	{
	case -1:
		LOG_LVL2("Disabling Debug Renderer");
		m_rendererSystem->DisableDebugRenderer();
		break;

	case 0:
		m_rendererSystem->SetDebugRenderer<RaytraceMeshInfo>();
		break;
	case 1:
		m_rendererSystem->SetDebugRenderer<HierarchyVisualization>();
		break;

	default:
		LOG_ERROR("Unknown renderer index!");
		return;
	}
}

void Application::Run()
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

void Application::Update(ezTime timeSinceLastUpdate)
{
	m_window->PollWindowEvents();

	if (!m_scriptWaitsForMSE)
		m_scriptProcessing.ProcessCommandQueue(timeSinceLastUpdate.GetSeconds());

	Input();

	if (m_rendererSystem->GetActiveRenderer())
	{
		if (m_camera->Update(timeSinceLastUpdate))
			m_rendererSystem->SetCamera(*m_camera);


		m_window->SetTitle(m_rendererSystem->GetActiveRenderer()->GetName() + " - iteration: " + std::to_string(m_rendererSystem->GetIterationCount()) + " - time per frame " +
			std::to_string(timeSinceLastUpdate.GetMilliseconds()) + "ms (FPS: " + std::to_string(1.0f / timeSinceLastUpdate.GetSeconds()) + ")");
	}
	else
		m_window->SetTitle("No renderer!");
}

void Application::Draw()
{
	if (!m_rendererSystem->GetActiveRenderer())
		return;

	m_rendererSystem->Draw();

	if (!m_rendererSystem->IsDebugRendererActive())
	{
		m_window->DisplayHDRTexture(m_rendererSystem->GetBackbuffer(), m_rendererSystem->GetIterationCount());
		MSEChecks();
	}

	m_window->Present();
}

void Application::MSEChecks()
{
	++m_iterationSinceLastMSECheck;
	int mseCheckInterval = GlobalConfig::GetParameter("mseCheckInterval")[0].As<int>();
	if (mseCheckInterval > 0 && m_iterationSinceLastMSECheck >= static_cast<unsigned int>(mseCheckInterval))
	{
		m_iterationSinceLastMSECheck = 0;
		float mse = m_textureMSE->ComputeCurrentMSE(m_rendererSystem->GetBackbuffer(), m_rendererSystem->GetIterationCount());
		LOG_LVL2("Iteration " << m_rendererSystem->GetIterationCount() << " MSE: " << mse);

		if (m_scriptWaitsForMSE)
		{
			float goalMSE = GlobalConfig::GetParameter("waitMSE")[0].As<float>();
			if (goalMSE > 0)
			{
				if (mse != mse)
				{
					LOG_ERROR("MSE is NaN. Resuming script.");
					m_scriptWaitsForMSE = false;
				}
				else if (goalMSE > mse)
				{
					LOG_LVL2("MSE now below " << goalMSE << "! Resuming script");
					m_scriptWaitsForMSE = false;
				}
			}
			else
				m_scriptWaitsForMSE = false;
		}
	}
}

void Application::Input()
{
	// Save image on F10/PrintScreen
	if (m_window->WasButtonPressed(GLFW_KEY_PRINT_SCREEN) || m_window->WasButtonPressed(GLFW_KEY_F10))
		SaveImage();

	// Add more useful hotkeys on demand!
}

std::string Application::UIntToMinLengthString(int _number, int _minDigits)
{
	int zeros = std::max(0, _minDigits - static_cast<int>(ceil(log10(_number + 1))));
	std::string out = std::string(zeros, '0') + std::to_string(_number);
	return out;
}

void Application::SaveImage(const std::string& name)
{
	if (!m_rendererSystem->GetActiveRenderer())
		return;

	time_t t = time(0);   // get time now
	struct tm* now = localtime(&t);
	std::string date = UIntToMinLengthString(now->tm_mon + 1, 2) + "." + UIntToMinLengthString(now->tm_mday, 2) + " " +
		UIntToMinLengthString(now->tm_hour, 2) + "h" + UIntToMinLengthString(now->tm_min, 2) + "m" + std::to_string(now->tm_sec) + "s ";
	std::string filename = "../screenshots/" + date + m_rendererSystem->GetActiveRenderer()->GetName() + " " +
		std::to_string(m_rendererSystem->GetIterationCount()) + "it"
		".pfm";
	if (!name.empty())
		filename = name + " " + filename;
	gl::Texture2D& backbuffer = m_rendererSystem->GetBackbuffer();
	std::unique_ptr<ei::Vec4[]> imageData(new ei::Vec4[backbuffer.GetWidth() * backbuffer.GetHeight()]);
	backbuffer.ReadImage(0, gl::TextureReadFormat::RGBA, gl::TextureReadType::FLOAT, backbuffer.GetWidth() * backbuffer.GetHeight() * sizeof(ei::Vec4), imageData.get());
	if (WritePfm(imageData.get(), ei::IVec2(backbuffer.GetWidth(), backbuffer.GetHeight()), filename, m_rendererSystem->GetIterationCount()))
		LOG_LVL1("Wrote screenshot \"" + filename + "\"");
}
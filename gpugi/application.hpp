#pragma once

#include <string>
#include <memory>

#include "control/scriptprocessing.hpp"
#include "Time/Stopwatch.h"
#include "control/globalconfig.hpp"


class TextureMSE;
class RendererSystem;
class OutputWindow;
class InteractiveCamera;
class Scene;



class Application
{
public:
	Application(int argc, char** argv);
	~Application();

	/// Starts simulation loop.
	void Run();

private:

	/// Registers most script commands at GlobalConfig.
	void RegisterScriptCommands();
	

	void SwitchRenderer(const GlobalConfig::ParameterType& p);

	void SwitchDebugRenderer(const GlobalConfig::ParameterType& p);


	/// Takes care of inputs etc.
	void Update(ezTime timeSinceLastUpdate);
	/// Keeps renderer running and updates screen content.
	void Draw();



	void MSEChecks();

	void Input();

	std::string UIntToMinLengthString(int _number, int _minDigits);

	void SaveImage(const std::string& name = "");



	ScriptProcessing m_scriptProcessing;
	std::unique_ptr<RendererSystem> m_rendererSystem;
	std::unique_ptr<OutputWindow> m_window;
	std::unique_ptr<InteractiveCamera> m_camera;
    std::shared_ptr<Scene> m_scene;
	

	ezStopwatch m_stopwatch;
	bool m_shutdown;

	bool m_scriptWaitsForMSE;
	unsigned int m_iterationSinceLastMSECheck;
	std::shared_ptr<TextureMSE> m_textureMSE;
};
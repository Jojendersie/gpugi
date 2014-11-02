#include "scriptprocessing.hpp"
#include "globalconfig.hpp"
#include "../utilities/logger.hpp"
#include <fstream>
#include <thread>
#include <iostream>

ScriptProcessing::ScriptProcessing() : 
	m_scriptWaitSeconds(0.0f),
	m_scriptWaitIterations(0),
	m_consoleWindowThreadRunning(false)
{
}

void ScriptProcessing::RunScript(const std::string& _scriptFilename)
{
	std::ifstream file(_scriptFilename);
	if (file.bad() || file.fail())
	{
		LOG_ERROR("Failed to load script \"" + _scriptFilename + "\"");
		return;
	}

	std::string line;
	while (std::getline(file, line))
		m_scriptCommandQueue.push(line);
}

void ScriptProcessing::CommandWindowThread()
{
	while (m_consoleWindowThreadRunning)
	{
		std::string line;
		std::getline(std::cin, line);

		if (line.empty())
			continue;
		m_consoleCommandQueueMutex.lock();
		m_consoleCommandQueue.push(line);
		m_consoleCommandQueueMutex.unlock();
	}
}

void ScriptProcessing::StartConsoleWindowThread()
{
	if (m_consoleWindowObservationThread || m_consoleWindowThreadRunning)
	{
		LOG_ERROR("Command window thread was still running. Try to stop old thread...");
		StopConsoleWindowThread();
	}

	m_consoleWindowThreadRunning = true;
	m_consoleWindowObservationThread.reset(new std::thread(&ScriptProcessing::CommandWindowThread, this));
}

void ScriptProcessing::StopConsoleWindowThread()
{
	m_consoleWindowThreadRunning = false;
	if (m_consoleWindowObservationThread)
	{
		m_consoleWindowObservationThread->join();
		m_consoleWindowObservationThread.release();
	}
}

void ScriptProcessing::ParseCommand(std::string _commandLine, bool _fromScriptFile)
{
	_commandLine.erase(std::remove(_commandLine.begin(), _commandLine.end(), ' '), _commandLine.end());
	_commandLine.erase(std::remove(_commandLine.begin(), _commandLine.end(), '\t'), _commandLine.end());
	if (_commandLine.empty())
		return;

	// Primitive parsing.
	std::string::size_type equalityIdx = _commandLine.find('=');
	std::string name = _commandLine.substr(0, equalityIdx);
	GlobalConfig::ParameterType argumentList;

	std::string::size_type lastComma = equalityIdx;
	std::string::size_type nextComma;
	if (equalityIdx != std::string::npos)
	{
		do
		{
			nextComma = _commandLine.find(',', lastComma + 1);

			std::string paramlistPart;
			if (nextComma == std::string::npos)
				paramlistPart = _commandLine.substr(lastComma + 1);
			else
				paramlistPart = _commandLine.substr(lastComma + 1, nextComma - lastComma - 1);

			try
			{
				argumentList.push_back(GlobalConfig::ParameterElementType(paramlistPart));
			}
			catch (...)
			{
				LOG_ERROR("Invalid parameter!");
			}

			lastComma = nextComma;
		} while (nextComma != std::string::npos);
	}

	// Parse special file commands.
	if (_fromScriptFile)
	{
		if (name == "waitSeconds")
		{
			if (argumentList.size() != 1)
				LOG_ERROR("The waitSeconds command expects a single number as argument.");
			else
				m_scriptWaitSeconds = argumentList[0].As<float>();
			return;
		}
		else if (name == "waitIterations")
		{
			if (argumentList.size() != 1)
				LOG_ERROR("The waitIterations command expects a single number as argument.");
			else
				m_scriptWaitIterations = argumentList[0].As<unsigned int>();
			return;
		}
	}

	// Perform command - if the argument list is empty, but the parameter consists of at least one parameter, interpret the command as getter.
	try
	{
		GlobalConfig::ParameterType currentValue = GlobalConfig::GetParameter(name);
		if (argumentList.empty() && !currentValue.empty())
		{
			std::cout << "{";
			for (size_t i = 0; i < currentValue.size() - 1; ++i)
				std::cout << currentValue[i].As<std::string>() << ", ";
			std::cout << currentValue[currentValue.size() - 1].As<std::string>() << " }\n";
		}
		else
		{
			GlobalConfig::SetParameter(name, argumentList);
		}
	}
	catch (const std::invalid_argument& e)
	{
		LOG_ERROR(e.what());
	}
}

void ScriptProcessing::ProcessCommandQueue(double timeDelta, unsigned int iterationDelta)
{
	m_scriptWaitSeconds -= timeDelta;
	if (m_scriptWaitIterations > 0)
		m_scriptWaitIterations -= iterationDelta;

	// Execute script commands.
	while (m_scriptWaitSeconds <= 0.0f && m_scriptWaitIterations == 0 && !m_scriptCommandQueue.empty())
	{
		ParseCommand(m_scriptCommandQueue.front(), true);
		m_scriptCommandQueue.pop();
	}

	// Execute console commands.
	m_consoleCommandQueueMutex.lock();
	while (!m_consoleCommandQueue.empty())
	{
		ParseCommand(m_consoleCommandQueue.front(), false);
		m_consoleCommandQueue.pop();
	}
	m_consoleCommandQueueMutex.unlock();
}

#include "scriptprocessing.hpp"
#include "globalconfig.hpp"
#include "../utilities/logger.hpp"
#include <fstream>
#include <thread>
#include <iostream>


static std::unique_ptr<std::thread> m_commandWindowObservationThread;

static std::mutex m_commandQueueMutex;
static std::queue<std::string> m_commandQueue;

static bool commandWindowThreadRunning = false;

void ScriptProcessing::RunScript(const std::string& _scriptFilename)
{
	std::ifstream file(_scriptFilename);
	if (file.bad() || file.fail())
	{
		LOG_ERROR("Failed to load script \"" + _scriptFilename + "\"");
		return;
	}

	m_commandQueueMutex.lock();
	std::string line;
	while (std::getline(file, line))
		m_commandQueue.push(line);
	m_commandQueueMutex.unlock();
}

static void CommandWindowThread()
{
	while (commandWindowThreadRunning)
	{
		std::string line;
		std::getline(std::cin, line);

		if (line.empty())
			continue;
		m_commandQueueMutex.lock();
		m_commandQueue.push(line);
		m_commandQueueMutex.unlock();
	}
}

void ScriptProcessing::StartCommandWindowThread()
{
	if (m_commandWindowObservationThread || commandWindowThreadRunning)
	{
		LOG_ERROR("Command window thread was still running. Try to stop old thread...");
		StopCommandWindowThread();
	}

	commandWindowThreadRunning = true;
	m_commandWindowObservationThread.reset(new std::thread(CommandWindowThread));
}

void ScriptProcessing::StopCommandWindowThread()
{
	if (m_commandWindowObservationThread)
	{
		m_commandWindowObservationThread->join();
		m_commandWindowObservationThread.release();
	}
	commandWindowThreadRunning = false;
}

void ScriptProcessing::ProcessCommandQueue()
{
	m_commandQueueMutex.lock();
	while (!m_commandQueue.empty())
	{
		std::string command = m_commandQueue.front();
		command.erase(std::remove(command.begin(), command.end(), ' '), command.end());

		// Primitive parsing.
		std::string::size_type equalityIdx = command.find('=');
		std::string name = command.substr(0, equalityIdx);
		GlobalConfig::ParameterType parameter;

		std::string::size_type lastComma = equalityIdx;
		std::string::size_type nextComma;
		if (equalityIdx != std::string::npos)
		{
			do
			{
				nextComma = command.find(',', lastComma + 1);

				std::string paramlistPart;
				if (nextComma == std::string::npos)
					paramlistPart = command.substr(lastComma + 1);
				else
					paramlistPart = command.substr(lastComma + 1, nextComma - lastComma - 1);

				float value = 0.0f;
				try
				{
					value = std::stof(paramlistPart);
					parameter.push_back(value);
				}
				catch (...)
				{
					LOG_ERROR("Invalid parameter!");
				}

				lastComma = nextComma;
			} while (nextComma != std::string::npos);
		}

		// Perform command.
		try
		{
			GlobalConfig::SetParameter(name, parameter);
		}
		catch (const std::runtime_error&)
		{
			LOG_ERROR("Command name " + name + " does not exist!");
		}

		m_commandQueue.pop();
	}
	m_commandQueueMutex.unlock();
}

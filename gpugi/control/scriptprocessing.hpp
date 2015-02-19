#pragma once

#include <string>
#include <queue>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>

/// Allows changes/triggers in the global config by simple script commands.
///
/// Script commands are fed in either by console or a script file.
class ScriptProcessing
{
public:
	ScriptProcessing();
	~ScriptProcessing() {}

	/// Loads a sequence of commands from the given script file.
	///
	/// Scripts have to extra commands: waitSeconds, waitIterations
	/// Both take each a single number as argument and will delay the next commands by the given number of seconds/iterations.
	/// The console window (if activated via StartConsoleWindowThread) will continue to react as usual.
	void RunScript(const std::string& _scriptFilename);

	/// Starts a thread that listens to the command window and puts new commands to the command queue.
	void StartConsoleWindowThread();

	/// Stops thread started in StartConsoleWindowThread.
	void StopConsoleWindowThread();

	/// Parses and processes all commands in the process queue.
	void ProcessCommandQueue(double timeDelta, unsigned int iterationDelta = 1);

	/// Adds a command that causes ProcessCommandQueue to stop processing and return.
	///
	/// Remaining commands will be processed in the next call.
	void AddProcessingPauseCommand(const std::string& _processPauseCommand) { m_processPauseCommand.insert(_processPauseCommand); }

private:
	
	void ParseCommand(std::string command, bool fromScriptFile, bool& _stopProcessing);
	void CommandWindowThread();

	std::queue<std::string> m_scriptCommandQueue;
	double m_scriptWaitSeconds;
	unsigned int m_scriptWaitIterations;

	std::unique_ptr<std::thread> m_consoleWindowObservationThread;
	std::mutex m_consoleCommandQueueMutex;
	std::queue<std::string> m_consoleCommandQueue;

	std::unordered_set<std::string> m_processPauseCommand;

	bool m_consoleWindowThreadRunning;
};
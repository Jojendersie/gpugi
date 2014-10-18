#pragma once

#include <string>
#include <queue>
#include <memory>
#include <mutex>

namespace std
{
	class thread;
}

/// Allows changes/triggers in the global config by simple script commands.
///
/// Script commands are fed in either by console or a script file.
namespace ScriptProcessing
{
	/// Loads commands from the given script file and puts them directly into the command queue.
	void RunScript(const std::string& _scriptFilename);

	/// Starts a thread that listens to the command window and puts new commands to the command queue.
	void StartCommandWindowThread();

	/// Stops thread started in StartCommandWindowThread.
	void StopCommandWindowThread();

	/// Parses and processes all commands in the process queue.
	void ProcessCommandQueue();
};
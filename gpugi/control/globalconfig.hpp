#pragma once

#include <string>
#include <unordered_map>
#include <functional>


/// Primitive system for global config parameters.
///
/// Missuse as event system is appreciated - just specify an empty vector for an command.
namespace GlobalConfig
{
	typedef std::function<void(const std::vector<float>&)> SetFunc;
	typedef std::function<std::vector<float>()> GetFunc;
	typedef SetFunc ListenerFunc;
	typedef std::vector<float> ParameterType;

	/// Adds a new global parameter.
	///
	/// \param _name 
	///		Name of the parameter.
	/// \throws std::runtime_exception if parameter already exists.
	void AddParameter(const std::string& _name, const ParameterType& _value, const std::string& _description = "<no desc>");

	/// Adds a listener to an existing parameter.
	///
	/// \param _name 
	///		Name of the parameter.
	/// \param _listenerName
	///		Name of the listener. Must be unique among all listeners of this parameter.
	/// \param _listenerFunc
	///		Listener function called whenever the parameter's set function is called.
	/// \throws std::runtime_exception if the parameter does not exist or the listener name already exists.
	void AddListener(const std::string& _name, const std::string& _listenerName, const ListenerFunc& _listenerFunc);

	/// Removes an event listener to an existing parameter.
	///
	/// \param _name 
	///		Name of the parameter. Must be unique among all parameters.
	/// \param _listenerName
	///		Name of the listener. Must be unique among all listeners of this parameter.
	/// \throws std::runtime_exception if the parameter or the listener name does not exist.
	void RemovesListener(const std::string& _name, const std::string& _listenerName);


	/// Returns the current value of a given parameter.
	///
	/// \param _name 
	///		Name of the parameter. Must be unique among all parameters.
	/// \throws std::runtime_exception if the parameter does not exist.
	ParameterType GetParameter(const std::string& _name);

	/// Sets value of a given parameter and calls all listeners.
	///
	/// \param _name 
	///		Name of the parameter. Must be unique among all parameters.
	/// \throws std::runtime_exception if the parameter does not exist.
	void SetParameter(const std::string& _name, const ParameterType& _newValue);

	/// Returns a descriptive string of all entries.
	std::string GetEntryDescriptions();

}
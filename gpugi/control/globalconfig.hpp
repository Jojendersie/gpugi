#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include "../utilities/variant.hpp"


/// Primitive system for global config parameters.
///
/// Missuse as event system is appreciated - just specify an empty vector for an command.
namespace GlobalConfig
{
	typedef Variant<float, int, bool, std::string> ParameterElementType;
	typedef std::vector<ParameterElementType> ParameterType;
	typedef std::function<void(const ParameterType&)> SetFunc;
	typedef std::function<ParameterType()> GetFunc;
	typedef SetFunc ListenerFunc;

	/// Adds a new global parameter.
	///
	/// \param _name 
	///		Name of the parameter.
	/// \param _value
	///		Start value of the parameter. Future set calls will enforce the same _value.size().
	/// \throws std::invalid_argument if parameter already exists.
	void AddParameter(const std::string& _name, const ParameterType& _value, const std::string& _description = "<no desc>");

	/// Removes a new global parameter.
	///
	/// \param _name 
	///		Name of the parameter.
	/// \throws std::invalid_argument if parameter already exists.
	void RemoveParameter(const std::string& _name);

	/// Adds a listener to an existing parameter.
	///
	/// \param _name 
	///		Name of the parameter.
	/// \param _listenerName
	///		Name of the listener. Must be unique among all listeners of this parameter.
	/// \param _listenerFunc
	///		Listener function called whenever the parameter's set function is called.
	/// \throws std::invalid_argument if the parameter does not exist or the listener name already exists.
	void AddListener(const std::string& _name, const std::string& _listenerName, const ListenerFunc& _listenerFunc);

	/// Removes an event listener to an existing parameter.
	///
	/// \param _name 
	///		Name of the parameter. Must be unique among all parameters.
	/// \param _listenerName
	///		Name of the listener. Must be unique among all listeners of this parameter.
	/// \throws std::invalid_argument if the parameter or the listener name does not exist.
	void RemovesListener(const std::string& _name, const std::string& _listenerName);


	/// Returns the current value of a given parameter.
	///
	/// \param _name 
	///		Name of the parameter. Must be unique among all parameters.
	/// \throws std::invalid_argument if the parameter does not exist 
	ParameterType GetParameter(const std::string& _name);

	/// Sets value of a given parameter and calls all listeners.
	///
	/// \param _name 
	///		Name of the parameter. Must be unique among all parameters.
	/// \throws std::invalid_argument if the parameter does not exist or the number of values does not match the parameter's value count.
	void SetParameter(const std::string& _name, const ParameterType& _newValue);

	/// Returns a descriptive string of all entries.
	std::string GetEntryDescriptions();

}
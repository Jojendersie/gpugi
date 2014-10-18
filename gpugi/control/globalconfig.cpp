#include "globalconfig.hpp"

std::unordered_map<std::string, GlobalConfig::Entry> GlobalConfig::m_entries;

void GlobalConfig::AddParameter(const std::string& _name, const ParameterType& _value, const std::string& _description)
{
	Entry newEntry;
	newEntry.m_description = _description;
	newEntry.m_value = _value;
	auto it = m_entries.emplace(_name, newEntry);
	if (!it.second)
		throw std::runtime_error("Config entry with name \"" + _name + "\" does already exist.");
}

void GlobalConfig::AddListener(const std::string& _name, const std::string& _listenerName, const ListenerFunc& _listenerFunc)
{
	auto entry = m_entries.find(_name);
	if (entry != m_entries.end())
	{
		auto listener = entry->second.m_listeners.emplace(_listenerName, _listenerFunc);
		if (!listener.second)
			throw std::runtime_error("Config entry with name \"" + _name + "\" has already a listener with name \"" + _listenerName + "\".");
	}
	else
		throw std::runtime_error("Config entry with name \"" + _name + "\" does not exist.");
}

void GlobalConfig::RemovesListener(const std::string& _name, const std::string& _listenerName)
{
	auto entry = m_entries.find(_name);
	if (entry != m_entries.end())
	{
		auto listener = entry->second.m_listeners.find(_listenerName);
		if (listener == entry->second.m_listeners.end())
			throw std::runtime_error("Config entry with name \"" + _name + "\" does not have a listener with name \"" + _listenerName + "\".");
		entry->second.m_listeners.erase(listener);
	}
	else
		throw std::runtime_error("Config entry with name \"" + _name + "\" does not exist.");
}


GlobalConfig::ParameterType GlobalConfig::GetParameter(const std::string& _name)
{
	auto entry = m_entries.find(_name);
	if (entry == m_entries.end())
		throw std::runtime_error("Config entry with name \"" + _name + "\" does not exist.");
	else
		return entry->second.m_value;
}

void GlobalConfig::SetParameter(const std::string& _name, const ParameterType& _newValue)
{
	auto entry = m_entries.find(_name);
	if (entry == m_entries.end())
		throw std::runtime_error("Config entry with name \"" + _name + "\" does not exist.");
	else
	{
		entry->second.m_value = _newValue;

		for (auto it = entry->second.m_listeners.begin(); it != entry->second.m_listeners.end(); ++it)
		{
			it->second(_newValue);
		}
	}
}

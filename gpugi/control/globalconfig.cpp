#include "globalconfig.hpp"

struct Entry
{
	std::unordered_map<std::string, GlobalConfig::ListenerFunc> m_listeners;
	GlobalConfig::ParameterType m_value;
	std::string m_description;
};
static std::unordered_map<std::string, Entry> m_entries;


void GlobalConfig::AddParameter(const std::string& _name, const ParameterType& _value, const std::string& _description)
{
	Entry newEntry;
	newEntry.m_description = _description;
	newEntry.m_value = _value;
	auto it = m_entries.emplace(_name, newEntry);
	if (!it.second)
		throw std::invalid_argument("Config entry with name \"" + _name + "\" does already exist.");
}

void GlobalConfig::RemoveParameter(const std::string& _name)
{
	auto entry = m_entries.find(_name);
	if (entry == m_entries.end())
		throw std::invalid_argument("Config entry \"" + _name + "\" does not exist.");
	m_entries.erase(entry);
}

void GlobalConfig::AddListener(const std::string& _name, const std::string& _listenerName, const ListenerFunc& _listenerFunc)
{
	auto entry = m_entries.find(_name);
	if (entry != m_entries.end())
	{
		auto listener = entry->second.m_listeners.emplace(_listenerName, _listenerFunc);
		if (!listener.second)
			throw std::invalid_argument("Config entry with name \"" + _name + "\" has already a listener with name \"" + _listenerName + "\".");
	}
	else
		throw std::invalid_argument("Config entry with name \"" + _name + "\" does not exist.");
}

void GlobalConfig::RemovesListener(const std::string& _name, const std::string& _listenerName)
{
	auto entry = m_entries.find(_name);
	if (entry != m_entries.end())
	{
		auto listener = entry->second.m_listeners.find(_listenerName);
		if (listener == entry->second.m_listeners.end())
			throw std::invalid_argument("Config entry with name \"" + _name + "\" does not have a listener with name \"" + _listenerName + "\".");
		entry->second.m_listeners.erase(listener);
	}
	else
		throw std::invalid_argument("Config entry \"" + _name + "\" does not exist.");
}


GlobalConfig::ParameterType GlobalConfig::GetParameter(const std::string& _name)
{
	auto entry = m_entries.find(_name);
	if (entry == m_entries.end())
		throw std::invalid_argument("Config entry \"" + _name + "\" does not exist.");
	else
		return entry->second.m_value;
}

void GlobalConfig::SetParameter(const std::string& _name, const ParameterType& _newValue)
{
	auto entry = m_entries.find(_name);
	if (entry == m_entries.end())
		throw std::invalid_argument("Config entry \"" + _name + "\" does not exist.");
	else
	{
		if (_newValue.size() != entry->second.m_value.size())
			throw std::invalid_argument("Config entry \"" + _name + "\" expects " + std::to_string(entry->second.m_value.size()) + " parameters. Passed were " + std::to_string(_newValue.size()));

		entry->second.m_value = _newValue;

		for (auto it = entry->second.m_listeners.begin(); it != entry->second.m_listeners.end(); ++it)
		{
			it->second(_newValue);
		}
	}
}

std::string GlobalConfig::GetEntryDescriptions()
{
	std::string out;
	unsigned int descIdx = 0;
	for (auto desc = m_entries.begin(); desc != m_entries.end(); ++desc, ++descIdx)
	{
		out += "##### \"" + desc->first + "\"" + " (" + std::to_string(desc->second.m_value.size()) + " params)\n" + desc->second.m_description;
		if (!desc->second.m_listeners.empty())
		{
			out += "\n[Listeners: ";
			unsigned int listenerIdx = 0;
			for (auto listener = desc->second.m_listeners.begin(); listener != desc->second.m_listeners.end(); ++listener, ++listenerIdx)
			{
				out += "\"" + listener->first + "\"";
				if (listenerIdx < desc->second.m_listeners.size() - 1)
					out += ", ";
			}
			out += "]";
		}
		if (descIdx < m_entries.size() - 1)
			out += "\n\n";
		
	}
	return out;
}
#pragma once

#include "policy.hpp"
#include <cstdio>
#include <iostream>

namespace Logger {

	FilePolicy::FilePolicy(const std::string& _fileName) : m_file(_fileName)
	{
		if( !m_file )
			throw std::runtime_error("Cannot open file '" + _fileName + "' for logging.");
	}
	
	void FilePolicy::Write( const std::string& _message )
	{
		m_file << _message;
		std::cout << _message;
	}

} // namespace Logger
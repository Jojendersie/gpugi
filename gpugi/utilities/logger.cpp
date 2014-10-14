#include "logger.hpp"
#include <sstream>

namespace Logger {

	// ********************************************************************* //
	std::string Logger::GetTimeString()
	{
		std::string timeStr;
		time_t rawTime;
		time( &rawTime );
		timeStr = ctime( &rawTime );
		return timeStr.substr( 0 , timeStr.size() - 1 );
	}

	// ********************************************************************* //
	Logger::~Logger()
	{
		m_policy->Write( GetTimeString() );
		m_policy->Write( " Closing Logger\n" );

		// If there is some writing do not delete the policy instant.
		m_mutex.lock();
		delete m_policy;
		m_policy = nullptr;
		m_mutex.unlock();
	}
	
	// ********************************************************************* //
	void Logger::Initialize( Policy* _policy )
	{
		delete m_policy;
		m_policy = _policy;
		m_policy->Write( GetTimeString() );
		m_policy->Write( " Initialized Logger\n" );
	}
	
	// ********************************************************************* //
	void Logger::Write(const char* _file, const char* _function, long _codeLine, const char* _severity, const std::string& _message)
	{
		// Add a header to the message from the user
		std::stringstream message;	message.str("");

		// Shorten the full file name done to just the file
		const char* fileShort = strrchr(_file, '\\')+1;
		if( !fileShort ) fileShort = strrchr(_file, '/')+1;
		if( !fileShort ) fileShort = _file;

		message << _severity << " < ";
		// Write millisecond since start at the beginning
		message.fill('0');
		message.width(8);
		message << clock() << " ms > [";
		message << _function << " @ " << fileShort <<  " : ";
//		message.setf( std::ios::left );
//		message.fill(' ');
//		message.width(4);
		message << _codeLine << "]      ";
		message << _message << '\n';//*/
		if( m_policy ) m_policy->Write( message.str() );
	}

	// ********************************************************************* //
	void Logger::Write(const char* _severity, const std::string& _message)
	{
		// Add a header to the message from the user
		std::stringstream message;	message.str("");

		message << _severity << " < ";
		// Write millisecond since start at the beginning
		message.fill('0');
		message.width(8);
		message << clock() << " ms >    ";
		message << _message << '\n';
		m_policy->Write( message.str() );
	}

} // namespace Logger
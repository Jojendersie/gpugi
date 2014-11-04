#pragma once

#include <cstdint>
#include <string>
#include "file.hpp"

namespace Jo {
namespace Files {

	/// \brief A collection of functions to make reading formated things from
	///		a file easier.
	namespace StreamReader
	{
		/// \brief Advance the file pointer until a non-whitespace is found.
		///	\details Afterwards the file pointer points to the position after
		///		 the first non-whitespace.
		/// \return The value of the first non-whitespace character.
		/// \throws 0 if eof is reached before finding something.
		uint8_t SkipWhitespaces( const IFile& _file );

		/// \brief Read stuff up to the next whitespace. Whitespaces at the
		///		beginning are ignored.
		///	\details The file pointer points to one after the separating
		///		whitespace.
		void ReadWord( const IFile& _file, std::string& _out );

		/// \brief Read stuff up to the end of the line, all whitespaces
		///		included except the Win-Carriage-Return if existing.
		void ReadLine( const IFile& _file, std::string& _out );

		/// \brief Interpret text from a stream as float. Whitespaces at the
		///		beginning are ignored.
		float ReadASCIIFloat( const IFile& _file );

		/// \brief Interpret text from a stream as int. Whitespaces at the
		///		beginning are ignored.
		///	\details If there is no number (other characters) the return value
		///		is 0. The file cursor points to the first character which does
		///		not belong to the interpreted int afterwards.
		int ReadASCIIInt( const IFile& _file );

	} // namespace StreamReader

} // namespace Jo
} // namespace Files
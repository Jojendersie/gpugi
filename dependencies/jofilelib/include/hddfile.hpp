#pragma once

#include "file.hpp"
#include <string>
#include <cstdio>

namespace Jo {
namespace Files {

	/**************************************************************************//**
	 * \class	Files::HDD
	 * \brief	Read and write in a real file on hard disk.
	 * \details	The file is buffered with a 4096 byte buffer.
	 *****************************************************************************/
	class HDDFile: public IFile
	{
	protected:
		/// \brief C standard library file which is wrapped.
		FILE* m_file;
		int m_pendingWriteBytes;
	public:
		/// \brief Determine how a file should be opened.
		/// \details The mode flags can be used in any combination.
		typedef int ModeFlags;
		static const int OVERWRITE = 1;		///< Clear the file on opening. Create the file inclusive its path if not existent.
		static const int APPEND = 2;		///< Set cursor to the end of file (standard is at the beginning)

		/// \brief Open a file on hard disk.
		/// \details If the file/directory does not exist it will be created or
		///		not depending on _failIfNotExistent.
		/// \param [in] _name Name and path to a file on disk.
		/// \param [in] _failIfNotExistent Try to open file in read+write mode. This
		///		will fail with an exception if the file does not exists. To
		///		open a file may also fail if permission is denied.
		/// \param [in] _bufferSize Write operations are done buffered where
		///		the default buffer size is 4KB.
		HDDFile( const std::string& _name, ModeFlags _flags = 0, int _bufferSize = 4096 );

		/// \brief Move construction
		HDDFile(HDDFile&& _file);

		~HDDFile();

		/// \brief Close the old file and take the new reference.
		const HDDFile& operator = (HDDFile&& _file);

		virtual void Read( uint64_t _numBytes, void* _to ) const override;
		virtual uint8_t Next() const override;
		virtual void Write( const void* _from, uint64_t _numBytes ) override;

		/// \details Seek can only jump within the existing file.
		virtual void Seek( uint64_t _numBytes, SeekMode _mode = SeekMode::SET ) const override;

		virtual std::string Name() const override { return m_name; }

		/// \brief Write the buffer to disk if there are written bytes pending.
		void Flush();

	private:
		// Copying files not allowed.
		void operator = (const HDDFile&);
		HDDFile(const HDDFile&);

		std::string m_name;
	};
	
} // namespace Files
} // namespace Jo
#pragma once

#include "file.hpp"

namespace Jo {
namespace Files {

	/**************************************************************************//**
	 * \class	Files::MemFile
	 * \brief	Set a default memory to be interpreted as a file.
	 * \details	
	 *****************************************************************************/
	class MemFile: public IFile
	{
	protected:
		/// \brief The whole file in memory.
		/// \details The buffer can have a different capacity than it is filled
		///		with real data.
		void* m_buffer;
		uint64_t m_capacity;
		/// \brief False if the file wraps an external memory -> memory will
		///		not be deleted with the file.
		bool m_ownsMemory;
	public:
		/// \brief Uses an existing memory for file read access.
		/// \param [in] _memory The memory which should be wrapped. Do not
		///		delete before this MemFile. The file will access directly to
		///		the original memory.
		/// \param [in] _size Size of the part which should be wrapped. This
		///		must not necessarily be the whole memory.
		MemFile( const void* _memory, uint64_t _size );

		/// \brief Creates a file of size 0 with read and write access.
		/// \param [in] _capacity Initial capacity. The buffer is resized if
		///		necessary. The default value is 4 KB. If you can (over)
		///		estimate the target size this will have higher performances.
		MemFile( uint64_t _capacity = 4096 );

		/// \brief Move construction
		MemFile( MemFile&& _file );

		~MemFile();

		/// \brief Forget old memory and take new file.
		/// \warning Are you sure you will overwrite the current data?
		const MemFile& operator = ( MemFile&& _file );

		virtual void Read( uint64_t _numBytes, void* _to ) const override;
		virtual uint8_t Next() const override;
		virtual void Write( const void* _from, uint64_t _numBytes ) override;

		/// \brief Make sure that a direct write to the returned address does
		///		not cause a buffer overflow.
		///	\details This method can be used for direct writes to the MemFile.
		///		Which is usefull if another method expects a buffer to write
		///		into. E.g.:
		///			void* buffer = memfile.Reserve(100);
		///			fread( file, 100, 1, buffer );
		///			
		///		The cursor and the size will be moved by the given size as if
		///		a write operation took place.
		void* Reserve( uint64_t _numBytes );

		/// \details Seek can even jump to locations > size for random write
		///		access. Reading at such a location will fail.
		virtual void Seek( uint64_t _numBytes, SeekMode _mode = SeekMode::SET ) const override;

		/// \brief The name is the address of the internal pointer
		virtual std::string Name() const override;

		//void* GetBuffer()				{ return m_buffer; }
		const void* GetBuffer() const	{ return m_buffer; }

	private:
		// Copying files not allowed.
		void operator = (const MemFile&);
		MemFile(const MemFile&);
	};

} // namespace Files
} // namespace Jo
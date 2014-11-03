#pragma once

#include "gl.hpp"

#include <cstdint>

namespace gl
{
    /// A buffer is a pure GPU sided storage.
    /// \details Buffers are used in many ways in OpenGL. This class provides
    ///     an interface to this raw memory blocks.
    ///
    ///     This buffer is always created immutable so it cannot be changed in
    ///     its size, but read and written.
    ///
    ///     Currently you can use the TextureBufferView on buffers.
    class Buffer
    {
    public:
		friend class UniformBufferView;
		friend class ShaderStorageBufferView;

        /// Flags for the (optimized) buffer creation.
        enum class Usage
        {
			IMMUTABLE = 0,
            MAP_WRITE = GL_MAP_WRITE_BIT,      ///< Enable CPU sided write via mapping.
            MAP_READ = GL_MAP_READ_BIT,       ///< Enable CPU sided read via mapping.
			
			/// Allows the buffer object to be mapped in such a way that it can be used while it is mapped.
			/// The buffer will be mapped entirely once. Calls to map only return the already mapped memory.
			/// See https://www.opengl.org/wiki/Buffer_Object#Persistent_mapping
            MAP_PERSISTENT = GL_MAP_PERSISTENT_BIT, 
			/// Only valid in combination with MAP_PERSISTENT. If active, glFlushMappedBufferRange can used instead of barrier.
			EXPLICIT_FLUSH = GL_MAP_FLUSH_EXPLICIT_BIT,
			/// Only valid in combination with MAP_PERSISTENT.
			/// Allows reads from and writes to a persistent buffer to be coherent with OpenGL, without an explicit barrier.
			/// Without this flag, you must use an explicit barrier to achieve coherency. You must use the persistent bit when using this bit.
            MAP_COHERENT = GL_MAP_COHERENT_BIT,   

            SUB_DATA_UPDATE = GL_DYNAMIC_STORAGE_BIT	///< Makes set and get available
        };

        /// Create and allocate the buffer
        Buffer( std::uint32_t _sizeInBytes, Usage _usageFlags, const void* _data = nullptr );

        ~Buffer();

		/// Requires manual move implementation to invalidate the m_bufferObject id
		/// before the destructor is called (the standard move deletes the resource
		/// here)
		Buffer( Buffer&& _rValue );

        /// Clear this buffer
        // Requires format data
        //void Clear(,,,);

		// TODO: Maps currently use creation-time defined mapping flag instead of user given flag.

        /// Maps the whole buffer.
		///
		/// \attention
		///		Returns mapped data pointer if entire buffer was already mapped. Unmaps buffer and writes warning to log if an incompatible area was already mapped.
		/// Performance: https://www.opengl.org/wiki/Buffer_Object#Mapping
		/// Map should be at least as fast as SubData, but is expected to be better.
		/// However writing sequentially is appreciated (as always with any piece of memory!).
		void* Map() { return Map(0, m_sizeInBytes); }

        /// Maps a part of the buffer.
		///
		/// \attention
		///		Returns mapped data pointer if desired buffer area was already mapped.
		///		Unmaps buffer and writes warning to log if an incompatible area was already mapped.
		/// \see Map
        void* Map(std::uint32_t _offset, std::uint32_t _numBytes);

		/// Unmaps the buffer.
		///
		/// Writes a warning to the log and ignores call if:
		/// * ... buffer was not mapped.
		/// * ... buffer was created with MAP_PERSISTENT, since buffer can stay mapped virtually forever. 
		/// (Later means that you have to take care of synchronizations yourself. 
		/// Either via glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT) or glFlushMappedBufferRange.)
        void Unmap();

        /// Use glBufferSubData to update a range in the buffer.
		/// \remarks
		///		The Map methods are usually faster.
		/// \see Map, Set
		void Set(const void* _data, std::uint32_t _numBytes, std::uint32_t _offset);

        /// Use glGetBufferSubData to update get a data range in the buffer.
		/// \attention
		///		Usage usually not recommend. Consider to use Map.
		void Get(void* _data, std::uint32_t _offset, std::uint32_t _numBytes);

		Usage GetUsageFlags() const		{ return m_usageFlags; }
        BufferId GetBufferId() const    { return m_bufferObject; }
        std::uint32_t GetSize() const   { return m_sizeInBytes; }

    private:
        BufferId m_bufferObject;
        std::uint32_t m_sizeInBytes;
        GLenum m_glMapAccess;   ///< READ_ONLY, WRITE_ONLY or READ_WRITE, and optionally GL_MAP_PERSISTENT_BIT.
        Usage m_usageFlags;			///< The newer glMapBufferRange requires the same bits as the construction

		std::uint32_t m_mappedDataOffset;
		std::uint32_t m_mappedDataSize;
		void* m_mappedData;

		// Non copy able
		Buffer( const Buffer& );
		void operator = ( const Buffer& );
    };
}
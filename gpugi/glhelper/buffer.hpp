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
        /// Flags for the (optimized) buffer creation.
        enum class Usage
        {
            WRITE = GL_MAP_WRITE_BIT,      ///< CPU sided write via mapping
            READ = GL_MAP_READ_BIT,       ///< CPU sided read via mapping
         //   PERSISTENT = GL_PERSISTENT_BIT, ///< Allows the buffer object to be mapped in such a way that it can be used while it is mapped. Requires READ or WRITE flag und manual barriers.
         //   COHERENT = GL_COHERENT_BIT,   ///< Allows reads from and writes to a persistent buffer to be coherent with OpenGL, without an explicit barrier. Without this flag, you must use an explicit barrier to achieve coherency. You must use the persistent bit when using this bit.
            SUB_DATA_UPDATE = 16
        };

        /// Create and allocate the buffer
        Buffer( std::uint32_t _sizeInBytes, Usage _usageFlags, const void* _data = nullptr );

        ~Buffer();

        /// Clear this buffer
        // Requires format data
        //void Clear(,,,);

        /// Map the whole buffer
        void* Map();

        /// Map a part of the buffer
        /// Performance: https://www.opengl.org/wiki/Buffer_Object#Mapping
        /// Map should be at least as fast as SubData, but is expected to
        /// be better. However writing sequentially is appreciated.
        void* Map( std::uint32_t _offset, std::uint32_t _numBytes );

        void Unmap();

        /// Use glBufferSubData to update a range in the buffer
        void Set( std::uint32_t _offset, std::uint32_t _numBytes, const void* _data );

        /// Uses Map() and Unmap() internally (usage not recommended)
        void Get( std::uint32_t _offset, std::uint32_t _numBytes, void* _data );

    private:
        BufferId m_bufferObject;
        std::uint32_t m_sizeInBytes;
        GLenum m_mapAccess;     ///< READ_ONLY, WRITE_ONLY or READ_WRITE
    };
}
#include "buffer.hpp"
#include "..\utilities\logger.hpp"

namespace gl {

    Buffer::Buffer( std::uint32_t _sizeInBytes, Usage _usageFlags, const void* _data ) :
        m_sizeInBytes(_sizeInBytes),
        m_mapAccessBits(_usageFlags)
    {
        GL_CALL(glCreateBuffers, 1, &m_bufferObject);
        GL_CALL(glNamedBufferStorage, m_bufferObject, _sizeInBytes, _data, GLbitfield(_usageFlags));

        if( uint32_t(_usageFlags) & uint32_t(Usage::READ) )
            m_mapAccess = uint32_t(_usageFlags) & uint32_t(Usage::WRITE) ? GL_READ_WRITE : GL_READ_ONLY;
        else m_mapAccess = uint32_t(_usageFlags) & uint32_t(Usage::WRITE) ? GL_WRITE_ONLY : 0;
    }

    Buffer::~Buffer()
    {
        GL_CALL(glDeleteBuffers, 1, &m_bufferObject);
    }

    void* Buffer::Map()
    {
        if( m_mapAccess == 0 )
            LOG_ERROR( "The buffer was not created with READ or WRITE flag. Unable to map memory!" );
        else
            return glMapNamedBuffer(m_bufferObject, m_mapAccess);
        return nullptr;
    }

    void* Buffer::Map( std::uint32_t _offset, std::uint32_t _numBytes )
    {
        if( m_mapAccess == 0 )
            LOG_ERROR( "The buffer was not created with READ or WRITE flag. Unable to map memory!" );
        else
            return glMapNamedBufferRange(m_bufferObject, _offset, _numBytes, GLbitfield(m_mapAccessBits));
        return nullptr;
    }

    void Buffer::Unmap()
    {
        GL_CALL(glUnmapNamedBuffer, m_bufferObject);
    }


    void Buffer::Set( std::uint32_t _offset, std::uint32_t _numBytes, const void* _data )
    {
        GL_CALL(glNamedBufferSubData, m_bufferObject, _offset, _numBytes, _data);
    }

    void Buffer::Get( std::uint32_t _offset, std::uint32_t _numBytes, void* _data )
    {
        memcpy(_data, Map( _offset, _numBytes ), _numBytes);
        Unmap();
    }
}

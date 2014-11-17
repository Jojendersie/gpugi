#include "texturebuffer.hpp"
#include "shaderobject.hpp"
#include "../utilities/logger.hpp"

namespace gl
{
	const TextureBufferView* TextureBufferView::s_boundTBOs[16];

	TextureBufferView::TextureBufferView() :
		m_textureObject(0)
	{
        GL_CALL(glCreateTextures, GL_TEXTURE_BUFFER, 1, &m_textureObject);
	}

	TextureBufferView::~TextureBufferView()
	{
        GL_CALL(glDeleteTextures, 1, &m_textureObject);
	}

	Result TextureBufferView::Init(std::shared_ptr<Buffer> _buffer, TextureBufferFormat _format)
	{
		return Init(_buffer, _format, 0, _buffer->GetSize());
	}

	Result TextureBufferView::Init(std::shared_ptr<Buffer> _buffer, TextureBufferFormat _format, std::uint32_t _offset, std::uint32_t _numBytes)
    {
        m_buffer = _buffer;
		GL_CALL(glTextureBufferRange, m_textureObject,
			static_cast<GLenum>(_format), _buffer->GetBufferId(), _offset, _numBytes);

        return SUCCEEDED;
    }

	void TextureBufferView::BindBuffer(GLuint _locationIndex) const
	{
		Assert(_locationIndex < sizeof(s_boundTBOs) / sizeof(TextureBufferView*), 
			"Can't bind tbo to slot " + std::to_string(_locationIndex) + ". Maximum number of slots is " + std::to_string(sizeof(s_boundTBOs) / sizeof(TextureBufferView*)));

		if (s_boundTBOs[_locationIndex] != this)
		{
            GL_CALL(glActiveTexture, GL_TEXTURE0 + _locationIndex); 
			GL_CALL(glBindTexture, GL_TEXTURE_BUFFER, m_textureObject);
			s_boundTBOs[_locationIndex] = this;
		}

	}
}
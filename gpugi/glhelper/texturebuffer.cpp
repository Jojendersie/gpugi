#include "texturebuffer.hpp"
#include "shaderobject.hpp"
#include "../utilities/logger.hpp"

namespace gl
{
	const TextureBufferView* TextureBufferView::s_boundTBOs[16];

	TextureBufferView::TextureBufferView() :
		m_textureObject(0)
	{
		if(glCreateTextures)
			GL_CALL(glCreateTextures, GL_TEXTURE_BUFFER, 1, &m_textureObject);
		else
			GL_CALL(glGenTextures, 1, &m_textureObject);
	}

	TextureBufferView::~TextureBufferView()
	{
        GL_CALL(glDeleteTextures, 1, &m_textureObject);
	}

    Result TextureBufferView::Init(std::shared_ptr<Buffer> _buffer, TextureBufferFormat _format)
    {
        m_buffer = _buffer;
		if(glTextureBuffer)
		{
			GL_CALL(glTextureBuffer, m_textureObject, 
				static_cast<GLenum>(_format), _buffer->GetBufferId());
		} else {
			BindBuffer(0);
			GL_CALL(glTexBuffer, GL_TEXTURE_BUFFER, 
				static_cast<GLenum>(_format), _buffer->GetBufferId());
		}

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
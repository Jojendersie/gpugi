#include "texturebuffer.hpp"
#include "shaderobject.hpp"
#include "../utilities/logger.hpp"

namespace gl
{
	TextureBufferView* TextureBufferView::s_boundTBOs[16];

	TextureBufferView::TextureBufferView() :
		m_textureObject(0)
	{
        GL_CALL(glCreateTextures, GL_TEXTURE_BUFFER, 1, &m_textureObject);
	}

	TextureBufferView::~TextureBufferView()
	{
        glDeleteTextures(1, &m_textureObject);
	}

    Result TextureBufferView::Init(std::shared_ptr<Buffer> _buffer,
            TextureFormat _format)
    {
        m_buffer = _buffer;
        GL_CALL(glTextureBuffer,
            m_textureObject,
            TextureFormatToGLSizedInternal[int(_format)],
            _buffer->GetBufferId());

        return SUCCEEDED;
    }

	void TextureBufferView::BindBuffer(GLuint locationIndex)
	{
		Assert(locationIndex < sizeof(s_boundTBOs) / sizeof(TextureBufferView*), 
			"Can't bind tbo to slot " + std::to_string(locationIndex) + ". Maximum number of slots is " + std::to_string(sizeof(s_boundTBOs) / sizeof(TextureBufferView*)));

		if (s_boundTBOs[locationIndex] != this)
		{
            GL_CALL(glActiveTexture, locationIndex); 
			GL_CALL(glBindTexture, GL_TEXTURE_BUFFER, m_textureObject);
			s_boundTBOs[locationIndex] = this;
		}

	}
}
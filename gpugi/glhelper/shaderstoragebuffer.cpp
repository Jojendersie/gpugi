#include "shaderstoragebuffer.hpp"
#include "shaderobject.hpp"
#include "buffer.hpp"
#include "../utilities/logger.hpp"
#include "../utilities/flagoperators.hpp"


namespace gl
{
	const ShaderStorageBufferView* ShaderStorageBufferView::s_boundSSBOs[16];

	ShaderStorageBufferView::ShaderStorageBufferView()
	{
        // No additional resources required
	}

	ShaderStorageBufferView::~ShaderStorageBufferView()
	{
	}

    Result ShaderStorageBufferView::Init(std::shared_ptr<gl::Buffer> _buffer, const std::string& _name)
    {
        m_buffer = _buffer;
		m_name = _name;

        return SUCCEEDED;
    }

	void ShaderStorageBufferView::BindBuffer(GLuint _locationIndex)
	{
		Assert(_locationIndex < sizeof(s_boundSSBOs) / sizeof(ShaderStorageBufferView*), 
			"Can't bind shader object buffer to slot " << _locationIndex << ". Maximum number of slots is " << sizeof(s_boundSSBOs) / sizeof(ShaderStorageBufferView*));

		if (m_buffer->m_mappedData != nullptr && static_cast<GLenum>(m_buffer->m_usageFlags & Buffer::Usage::MAP_PERSISTENT) == 0)
			m_buffer->Unmap();

		if (s_boundSSBOs[_locationIndex] != this)
		{
            GL_CALL(glBindBufferBase, GL_SHADER_STORAGE_BUFFER, _locationIndex, m_buffer->GetBufferId());
			s_boundSSBOs[_locationIndex] = this;
		}
	}
}
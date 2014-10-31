#include "structuredbuffer.hpp"
#include "shaderobject.hpp"
#include "buffer.hpp"
#include "../utilities/logger.hpp"
#include "../utilities/flagoperators.hpp"


namespace gl
{
	const StructuredBufferView* StructuredBufferView::s_boundSSBOs[16];

	StructuredBufferView::StructuredBufferView()
	{
        // No additional resources required
	}

	StructuredBufferView::~StructuredBufferView()
	{
	}

    Result StructuredBufferView::Init(std::shared_ptr<gl::Buffer> _buffer, const std::string& _name)
    {
        m_buffer = _buffer;
		m_name = _name;

        return SUCCEEDED;
    }

	void StructuredBufferView::BindBuffer(GLuint _locationIndex)
	{
		Assert(_locationIndex < sizeof(s_boundSSBOs) / sizeof(StructuredBufferView*), 
			"Can't bind shader object buffer to slot " << _locationIndex << ". Maximum number of slots is " << sizeof(s_boundSSBOs) / sizeof(StructuredBufferView*));

		if (m_buffer->m_mappedData != nullptr && static_cast<GLenum>(m_buffer->m_usageFlags & Buffer::Usage::MAP_PERSISTENT) == 0)
			m_buffer->Unmap();

		if (s_boundSSBOs[_locationIndex] != this)
		{
            GL_CALL(glBindBufferBase, GL_SHADER_STORAGE_BUFFER, _locationIndex, m_buffer->GetBufferId());
			s_boundSSBOs[_locationIndex] = this;
		}
	}
}
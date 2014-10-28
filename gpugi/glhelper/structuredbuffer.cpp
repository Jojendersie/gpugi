#include "structuredbuffer.hpp"
#include "shaderobject.hpp"
#include "../utilities/logger.hpp"

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

    Result StructuredBufferView::Init(std::shared_ptr<gl::Buffer> _buffer,
            const std::string& _name)
    {
        m_buffer = _buffer;

        return SUCCEEDED;
    }

	void StructuredBufferView::BindBuffer(GLuint _locationIndex) const
	{
		Assert(_locationIndex < sizeof(s_boundSSBOs) / sizeof(StructuredBufferView*), 
			"Can't bind tbo to slot " + std::to_string(_locationIndex) + ". Maximum number of slots is " + std::to_string(sizeof(s_boundSSBOs) / sizeof(StructuredBufferView*)));

        if( m_mappedBuffer )
        {
            m_buffer->Unmap();
            m_mappedBuffer = nullptr;
        }

		if (s_boundSSBOs[_locationIndex] != this)
		{
            GL_CALL(glBindBufferBase, GL_SHADER_STORAGE_BUFFER, _locationIndex, m_buffer->GetBufferId());
			s_boundSSBOs[_locationIndex] = this;
		}
	}
}
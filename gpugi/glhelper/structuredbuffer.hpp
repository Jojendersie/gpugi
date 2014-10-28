#pragma once

#include "gl.hpp"

#include "../utilities/utils.hpp"
#include "../utilities/assert.hpp"
#include "buffer.hpp"
#include "shaderobject.hpp"

#include <memory>

namespace gl
{
    /// Large buffers with cached index access for the GP.
    /// OpenGL name: shader storage buffer object (SSBO)
	class StructuredBufferView
	{
	public:
        /// Create the buffer
		StructuredBufferView();
		~StructuredBufferView();
			
        /// Bind the buffer data to this structured TextureBuffer.
        /// \param [in] _name Name used in shader programs if explicit binding
        ///     is not used.
		Result Init(std::shared_ptr<gl::Buffer> _buffer,
            const std::string& _name);

        const std::string& GetBufferName() const { return m_name; }

		/// Updates gpu data if necessary and binds buffer if not already bound.
		void BindBuffer(GLuint _locationIndex) const;

        /// Get the internal bound buffer resource.
        std::shared_ptr<Buffer> GetBuffer() const   { return m_buffer; }

        /// Structured array access. Requires the internal buffer to be mapped.
        template<typename T>
        T& operator[](std::uint32_t _index);
        template<typename T>
        const T& operator[](std::uint32_t _index) const;

	private:
        std::shared_ptr<Buffer> m_buffer;
        std::string m_name;
        mutable void* m_mappedBuffer;

		/// Currently bound SSBOs - number is arbitrary!
		static const StructuredBufferView* s_boundSSBOs[16];
	};

#include "structuredbuffer.inl"
}
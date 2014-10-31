#pragma once

#include <memory>

#include "gl.hpp"

#include "../utilities/utils.hpp"
#include "../utilities/assert.hpp"
namespace gl
{
	class Buffer;

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
		Result Init(std::shared_ptr<gl::Buffer> _buffer, const std::string& _name);

		const std::string& GetBufferName() const { return m_name; }

		/// Binds buffer if not already bound.
		///
		/// Performs an Unmap if the buffer is currently maped.
		void BindBuffer(GLuint _locationIndex);

		/// Get the internal bound buffer resource.
		std::shared_ptr<Buffer> GetBuffer() const   { return m_buffer; }

	private:
		std::shared_ptr<Buffer> m_buffer;
		std::string m_name;

		/// Currently bound SSBOs - number is arbitrary!
		static const StructuredBufferView* s_boundSSBOs[16];
	};
}
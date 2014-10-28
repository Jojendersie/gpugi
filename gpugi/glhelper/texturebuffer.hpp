#pragma once

#include "gl.hpp"
#include "shaderdatametainfo.hpp"

#include "../utilities/utils.hpp"
#include "../utilities/assert.hpp"
#include "textureformats.hpp"
#include "buffer.hpp"

#include <initializer_list>
#include <algorithm>
#include <memory>

namespace gl
{
    /// Large buffers with cached index access for the GP.
	class TextureBufferView
	{
	public:
        /// Create the buffer
		TextureBufferView();
		~TextureBufferView();
			
        /// Allocate the memory and copy the data to GPU.
		Result Init(std::shared_ptr<Buffer> _buffer,
            TextureFormat _format);

		/// Binds buffer if not already bound.
		void BindBuffer(GLuint locationIndex);

        /// Get the internal bound buffer resource.
        std::shared_ptr<Buffer> GetBuffer() const   { return m_buffer; }

	private:
		BufferId m_textureObject;
        std::shared_ptr<Buffer> m_buffer;

		/// Currently bound TBOs - number is arbitrary!
		static TextureBufferView* s_boundTBOs[16];
	};
}
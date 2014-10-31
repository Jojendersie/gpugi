#pragma once

#include "gl.hpp"
#include "shaderdatametainfo.hpp"
#include "buffer.hpp"

#include "../utilities/utils.hpp"
#include "../utilities/assert.hpp"

#include <initializer_list>
#include <algorithm>
#include <memory>

namespace gl
{
	class UniformBufferView
	{
	public:
		UniformBufferView();
		~UniformBufferView();
		
		/// Init as view to existing buffer.
		///
		/// Buffer needs to have at least MAP_WRITE access!
		Result Init(std::shared_ptr<Buffer> _buffer, const std::string& _bufferName);

		/// Creates a new buffer without any meta infos.
		///
		/// Buffer needs to have at least MAP_WRITE access!
		Result Init(std::uint32_t _bufferSizeBytes, const std::string& _bufferName, Buffer::Usage bufferUsage = Buffer::Usage::MAP_WRITE);

		/// Create a uniform buffer that matches all the given meta infos. Performs sanity checks if there's something contradictory
		Result Init(std::initializer_list<const gl::ShaderObject*> shaders, const std::string& _bufferName, Buffer::Usage bufferUsage = Buffer::Usage::MAP_WRITE);
		Result Init(const gl::ShaderObject& _shader, const std::string& _bufferName, Buffer::Usage bufferUsage = Buffer::Usage::MAP_WRITE);


		class Variable : public gl::ShaderVariable < UniformVariableInfo >
		{
		public:
			Variable() : ShaderVariable(), m_uniformBuffer(NULL) { }
			Variable(const UniformVariableInfo& metaInfo, UniformBufferView* pUniformBuffer) :
				ShaderVariable(metaInfo), m_uniformBuffer(pUniformBuffer) {}
			

			void Set(const void* pData, std::uint32_t SizeInBytes) override;

			using gl::ShaderVariable<UniformVariableInfo>::Set;
		private:
			UniformBufferView* m_uniformBuffer;
		};


		/// Sets variable on currently mapped data block.
		///
		/// \attention
		///		Checks only via assert if memory area is actually mapped!
		void Set(const void* _data, std::uint32_t offset, std::uint32_t _sizeInBytes);

		bool ContainsVariable(const std::string& _variableName) const       { return m_variables.find(_variableName) != m_variables.end(); }
		UniformBufferView::Variable& operator[] (const std::string& sVariableName);

		/// Binds buffer if not already bound.
		///
		/// Performs an Unmap if the buffer is currently maped.
		void BindBuffer(GLuint locationIndex);

		const std::string& GetBufferName() const { return m_bufferName; }

		/// Get the internal bound buffer resource.
		std::shared_ptr<Buffer> GetBuffer() const   { return m_buffer; }

	private:
		std::shared_ptr<Buffer> m_buffer;
		std::string		m_bufferName;

		/// meta information
		std::unordered_map<std::string, Variable> m_variables;


		/// Currently bound UBOs - number is arbitrary!
		static UniformBufferView* s_boundUBOs[16];
	};

#include "UniformBuffer.inl"
}

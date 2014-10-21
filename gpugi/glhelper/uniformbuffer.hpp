#pragma once

#include "gl.hpp"
#include "shaderdatametainfo.hpp"

#include "../utilities/utils.hpp"
#include "../utilities/assert.hpp"

#include <initializer_list>
#include <algorithm>
#include <memory	>

namespace gl
{
	class UniformBuffer
	{
	public:
		UniformBuffer();
		~UniformBuffer();
			
		/// will try to create a uniform buffer that matches all the given meta infos. Performs sanity checks if there's something contradictory
		Result Init(std::initializer_list<const gl::ShaderObject*> shaders, const std::string& sBufferName);

		Result Init(const gl::ShaderObject& shader, const std::string& sBufferName);
		Result Init(std::uint32_t BufferSizeBytes, const std::string& sBufferName);

		class Variable : public gl::ShaderVariable < UniformVariableInfo >
		{
		public:
			Variable() : ShaderVariable(), m_pUniformBuffer(NULL) { }
			Variable(const UniformVariableInfo& metaInfo, UniformBuffer* pUniformBuffer) :
				ShaderVariable(metaInfo), m_pUniformBuffer(pUniformBuffer) {}

			void Set(const void* pData, std::uint32_t SizeInBytes) override;

			using gl::ShaderVariable<UniformVariableInfo>::Set;
		private:
			UniformBuffer* m_pUniformBuffer;
		};


		bool ContainsVariable(const std::string& sVariableName) const       { return m_variables.find(sVariableName) != m_variables.end(); }
		UniformBuffer::Variable& operator[] (const std::string& sVariableName);

		/// \brief Sets data in buffer directly.
		/// Given data block will be set dirty and copied with the next BindBuffer/UpdateGPUData call
		void SetData(const void* pData, std::uint32_t DataSize, std::uint32_t Offset);

		/// Updates gpu data if necessary and binds buffer if not already bound.
		void BindBuffer(GLuint locationIndex);

		const std::string& GetBufferName() const { return m_sBufferName; }


		/// \brief Updates gpu UBO with dirty marked data
		/// Buffer should be already binded. Will be performed by BindBuffer by default.
		void UpdateGPUData();

	private:

		BufferId    m_BufferObject;
		std::uint32_t    m_uiBufferSizeBytes;
		std::string    m_sBufferName;

		/// where the currently dirty range of the buffer starts (bytes)
		std::uint32_t m_bufferDirtyRangeStart;
		/// where the currently dirty range of the buffer ends (bytes)
		std::uint32_t m_bufferDirtyRangeEnd;
		/// local copy of the buffer data
		std::unique_ptr<std::int8_t[]> m_bufferData;

		/// meta information
		std::unordered_map<std::string, Variable> m_variables;


		/// Currently bound UBOs - number is arbitrary!
		static UniformBuffer* s_boundUBOs[16];
	};

#include "UniformBuffer.inl"
}


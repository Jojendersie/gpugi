#include "uniformbuffer.hpp"
#include "shaderobject.hpp"
#include "../utilities/logger.hpp"

namespace gl
{
	UniformBuffer* UniformBuffer::s_boundUBOs[16];

	UniformBuffer::UniformBuffer() :
		m_BufferObject(9999),
		m_uiBufferSizeBytes(0),
		m_variables(),
		m_sBufferName(""),
		m_bufferDirtyRangeEnd(0),
		m_bufferDirtyRangeStart(0)
	{
	}

	UniformBuffer::~UniformBuffer(void)
	{
		GL_CALL(glDeleteBuffers, 1, &m_BufferObject);
	}

	Result UniformBuffer::Init(std::uint32_t bufferSizeBytes, const std::string& bufferName)
	{
		m_sBufferName = bufferName;
		m_uiBufferSizeBytes = bufferSizeBytes;

		m_bufferData.reset(new int8_t[bufferSizeBytes]);
		m_bufferDirtyRangeEnd = m_bufferDirtyRangeStart = 0;

		GL_CALL(glCreateBuffers, 1, &m_BufferObject);
		// TODO: Using mapping could be faster! http://www.gamedev.net/topic/622685-constant-buffer-updatesubresource-vs-map/
		GL_CALL(glNamedBufferStorage, m_BufferObject, static_cast<GLsizeiptr>(bufferSizeBytes), nullptr, GL_DYNAMIC_STORAGE_BIT);

		return SUCCEEDED;
	}

	Result UniformBuffer::Init(const gl::ShaderObject& shader, const std::string& bufferName)
	{
		auto uniformBufferInfoIterator = shader.GetUniformBufferInfo().find(bufferName);
		if (uniformBufferInfoIterator == shader.GetUniformBufferInfo().end())
		{
			LOG_ERROR("Shader \"" + shader.GetName() + "\" doesn't contain a uniform buffer meta block info with the name \"" + bufferName + "\"!");
			return FAILURE;
		}

		for (auto it = uniformBufferInfoIterator->second.Variables.begin(); it != uniformBufferInfoIterator->second.Variables.end(); ++it)
			m_variables.emplace(it->first, Variable(it->second, this));

		return Init(uniformBufferInfoIterator->second.iBufferDataSizeByte, bufferName);
	}

	Result UniformBuffer::Init(std::initializer_list<const gl::ShaderObject*> metaInfos, const std::string& bufferName)
	{
		Assert(metaInfos.size() != 0, "Meta info lookup list is empty!");

		bool initialized = false;
		int i = 0;
		for (auto shaderObjectIt = metaInfos.begin(); shaderObjectIt != metaInfos.end(); ++shaderObjectIt, ++i)
		{
			if (*shaderObjectIt == NULL)
			{
				LOG_LVL2("ShaderObject \"" + (*shaderObjectIt)->GetName() + "\" in list for uniform buffer \"" + bufferName + "\" initialization doesn't contain the needed meta data! Skiping..");
				continue;
			}
			auto uniformBufferInfoIterator = (*shaderObjectIt)->GetUniformBufferInfo().find(bufferName);
			if (uniformBufferInfoIterator == (*shaderObjectIt)->GetUniformBufferInfo().end())
			{
				LOG_LVL2("ShaderObject \"" + (*shaderObjectIt)->GetName() + "\" in list for uniform buffer \"" + bufferName + "\" initialization doesn't contain the needed meta data! Skiping..");
				continue;
			}

			if (!initialized)
			{
				Result result = Init(**metaInfos.begin(), bufferName);
				if (result == SUCCEEDED)
					initialized = true;
				continue;
			}

			// Sanity check.
			if (uniformBufferInfoIterator->second.iBufferDataSizeByte != m_uiBufferSizeBytes)
			{
				LOG_LVL2("ShaderObject \"" + (*shaderObjectIt)->GetName() + "\" in list for uniform buffer \"" + bufferName + "\" initialization gives size " +
					std::to_string(uniformBufferInfoIterator->second.iBufferDataSizeByte) + ", first shader gave size " + std::to_string(m_uiBufferSizeBytes) + "! Skiping..");
				continue;
			}

			for (auto varIt = uniformBufferInfoIterator->second.Variables.begin(); varIt != uniformBufferInfoIterator->second.Variables.end(); ++varIt)
			{
				auto ownVarIt = m_variables.find(varIt->first);
				if (ownVarIt != m_variables.end())  // overlap
				{
					// Sanity check.
					const gl::UniformVariableInfo* ownVar = &ownVarIt->second.GetMetaInfo();
					const gl::UniformVariableInfo* otherVar = &varIt->second;
					if (memcmp(ownVar, otherVar, sizeof(gl::UniformVariableInfo)) != 0)
					{
						LOG_ERROR("ShaderObject \"" + (*shaderObjectIt)->GetName() + "\" in list for uniform buffer \"" + bufferName + "\" has a description of variable \"" +
							varIt->first + "\" that doesn't match with the ones before!");
					}
				}
				else // new one
				{
					m_variables.emplace(varIt->first, Variable(varIt->second, this));
					// No overlap checking so far.
				}
			}
		}

		return SUCCEEDED;
	}

	void UniformBuffer::UpdateGPUData()
	{
		if (m_bufferDirtyRangeEnd <= m_bufferDirtyRangeStart)
			return;

		// TODO: Mapping could be faster! http://www.gamedev.net/topic/622685-constant-buffer-updatesubresource-vs-map/
		// See also here: https://www.opengl.org/wiki/Buffer_Object#Mapping .. can get rather complicated with all that syncing..
		GL_CALL(glNamedBufferSubData, m_BufferObject, m_bufferDirtyRangeStart, m_bufferDirtyRangeEnd - m_bufferDirtyRangeStart, m_bufferData.get() + m_bufferDirtyRangeStart);

		m_bufferDirtyRangeEnd = std::numeric_limits<std::uint32_t>::min();
		m_bufferDirtyRangeStart = std::numeric_limits<std::uint32_t>::max();
	}

	void UniformBuffer::BindBuffer(GLuint locationIndex)
	{
		Assert(locationIndex < sizeof(s_boundUBOs) / sizeof(UniformBuffer*), 
			"Can't bind ubo to slot " + std::to_string(locationIndex) + ". Maximum number of slots is " + std::to_string(sizeof(s_boundUBOs) / sizeof(UniformBuffer*)));

		if (s_boundUBOs[locationIndex] != this)
		{
			GL_CALL(glBindBufferBase, GL_UNIFORM_BUFFER, locationIndex, m_BufferObject);
			s_boundUBOs[locationIndex] = this;
		}

	}
}
#include "uniformbuffer.hpp"
#include "shaderobject.hpp"
#include "buffer.hpp"
#include "../utilities/logger.hpp"
#include "../utilities/flagoperators.hpp"

namespace gl
{
	UniformBufferView* UniformBufferView::s_boundUBOs[16];

	UniformBufferView::UniformBufferView() :
		m_variables(),
		m_bufferName("")
	{
	}

	UniformBufferView::~UniformBufferView()
	{
	}

	Result UniformBufferView::Init(std::shared_ptr<Buffer> _buffer, const std::string& _bufferName)
	{
		if (static_cast<std::uint32_t>(_buffer->GetUsageFlags() & Buffer::Usage::MAP_WRITE))
		{
			LOG_ERROR("Uniform buffer need at least Buffer::Usage::WRITE!");
			return FAILURE;
		}
		else
		{
			m_bufferName = _bufferName;
			m_buffer = _buffer;
			return SUCCEEDED;
		}
	}

	Result UniformBufferView::Init(std::uint32_t _bufferSizeBytes, const std::string& _bufferName, Buffer::Usage _bufferUsage)
	{
		if (static_cast<std::uint32_t>(_bufferUsage & Buffer::Usage::MAP_WRITE) == 0)
		{
			LOG_ERROR("Uniform buffer need at least Buffer::Usage::WRITE!");
			return FAILURE;
		}
		else
		{
			m_bufferName = _bufferName;
			m_buffer.reset(new Buffer(_bufferSizeBytes, _bufferUsage));
			return SUCCEEDED;
		}
	}

	Result UniformBufferView::Init(const gl::ShaderObject& _shader, const std::string& _bufferName, Buffer::Usage _bufferUsage)
	{
		auto uniformBufferInfoIterator = _shader.GetUniformBufferInfo().find(_bufferName);
		if (uniformBufferInfoIterator == _shader.GetUniformBufferInfo().end())
		{
			LOG_ERROR("Shader \"" + _shader.GetName() + "\" doesn't contain a uniform buffer meta block info with the name \"" + _bufferName + "\"!");
			return FAILURE;
		}

		for (auto it = uniformBufferInfoIterator->second.Variables.begin(); it != uniformBufferInfoIterator->second.Variables.end(); ++it)
			m_variables.emplace(it->first, Variable(it->second, this));

		return Init(uniformBufferInfoIterator->second.iBufferDataSizeByte, _bufferName, _bufferUsage);
	}

	Result UniformBufferView::Init(std::initializer_list<const gl::ShaderObject*> metaInfos, const std::string& bufferName, Buffer::Usage bufferUsage)
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
				Result result = Init(**metaInfos.begin(), bufferName, bufferUsage);
				if (result == SUCCEEDED)
					initialized = true;
				
				continue;
			}

			// Sanity check.
			if (uniformBufferInfoIterator->second.iBufferDataSizeByte != m_buffer->GetSize())
			{
				LOG_LVL2("ShaderObject \"" << (*shaderObjectIt)->GetName() << "\" in list for uniform buffer \"" << bufferName << "\" initialization gives size " <<
						uniformBufferInfoIterator->second.iBufferDataSizeByte << ", first shader gave size " << m_buffer->GetSize() << "! Skiping..");
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

		return m_buffer != nullptr ? SUCCEEDED : FAILURE;
	}

	void UniformBufferView::BindBuffer(GLuint locationIndex)
	{
		Assert(locationIndex < sizeof(s_boundUBOs) / sizeof(UniformBufferView*), 
			"Can't bind ubo to slot " + std::to_string(locationIndex) + ". Maximum number of slots is " + std::to_string(sizeof(s_boundUBOs) / sizeof(UniformBufferView*)));

		if (m_buffer->m_mappedData != nullptr && static_cast<GLenum>(m_buffer->m_usageFlags & Buffer::Usage::MAP_PERSISTENT) == 0)
			m_buffer->Unmap();

		if (s_boundUBOs[locationIndex] != this)
		{
			GL_CALL(glBindBufferBase, GL_UNIFORM_BUFFER, locationIndex, m_buffer->GetBufferId());
			s_boundUBOs[locationIndex] = this;
		}

	}

	void UniformBufferView::Set(const void* _data, std::uint32_t _offset, std::uint32_t _dataSize)
	{
		Assert(m_buffer != nullptr, "Uniform buffer " << m_bufferName << " is not initialized");
		Assert(_dataSize != 0, "Given size to set for uniform data is 0.");
		Assert(_offset + _dataSize <= m_buffer->GetSize(), "Data block doesn't fit into uniform buffer.");
		Assert(_data != nullptr, "Data to copy into uniform is nullptr.");


		Assert(m_buffer->m_mappedData != nullptr, "Buffer is not mapped!");
		Assert(m_buffer->m_mappedDataOffset <= _offset && m_buffer->m_mappedDataOffset + m_buffer->m_mappedDataSize >= _dataSize + _offset, "Buffer mapping range is not sufficient.");

		memcpy(static_cast<std::uint8_t*>(m_buffer->m_mappedData) + _offset, reinterpret_cast<const char*>(_data), _dataSize);
	}

}
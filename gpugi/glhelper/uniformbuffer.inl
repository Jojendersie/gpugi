
inline void UniformBuffer::Variable::Set(const void* pData, std::uint32_t sizeInBytes)
{
	Assert(m_pUniformBuffer != NULL, "Uniform buffer variable is not assigned to an Uniform Buffer!");
	Assert(sizeInBytes != 0, "Given size to set for uniform variable is 0.");
	Assert(pData != NULL, "Data to set for uniform variable is NULL.");

	m_pUniformBuffer->SetData(pData, sizeInBytes, m_MetaInfo.iBlockOffset);
}

inline void UniformBuffer::SetData(const void* pData, std::uint32_t dataSize, std::uint32_t offset)
{
	Assert(dataSize != 0, "Given size to set for uniform data is 0.");
	Assert(offset + dataSize <= m_uiBufferSizeBytes, "Data block doesn't fit into uniform buffer.");
	Assert(pData != NULL, "Data to copy into uniform is NULL.");

	m_bufferDirtyRangeStart = std::min(m_bufferDirtyRangeStart, offset);
	m_bufferDirtyRangeEnd = std::max(m_bufferDirtyRangeEnd, offset + dataSize);
	memcpy(m_bufferData.get() + offset, reinterpret_cast<const char*>(pData), dataSize);
}

inline UniformBuffer::Variable& UniformBuffer::operator[] (const std::string& sVariableName)
{
	Assert(m_variables.find(sVariableName) != m_variables.end(), "There is no variable named \"" + sVariableName + "\" in the uniform buffer \"" + m_sBufferName + " \"");
	return m_variables[sVariableName];
}

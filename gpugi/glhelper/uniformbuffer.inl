
inline void UniformBufferView::Variable::Set(const void* pData, std::uint32_t sizeInBytes)
{
	Assert(m_uniformBuffer != NULL, "Uniform buffer variable is not assigned to an Uniform Buffer!");
	Assert(m_uniformBuffer->m_buffer != nullptr, "Uniform buffer " << m_uniformBuffer->m_bufferName << " is not initialized");
	Assert(sizeInBytes != 0, "Given size to set for uniform variable is 0.");
	Assert(pData != NULL, "Data to set for uniform variable is NULL.");

	m_uniformBuffer->Set(pData, m_MetaInfo.iBlockOffset, sizeInBytes);
}

inline UniformBufferView::Variable& UniformBufferView::operator[] (const std::string& sVariableName)
{
	Assert(m_buffer != nullptr, "Uniform buffer " << m_bufferName << " is not initialized");
	Assert(m_variables.find(sVariableName) != m_variables.end(), "There is no variable named \"" + sVariableName + "\" in the uniform buffer \"" + m_bufferName + " \"");
	return m_variables[sVariableName];
}

template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(float f)
{
	Assert(m_MetaInfo.Type == ShaderVariableType::FLOAT, "Variable type does not match!");
	Set(&f, sizeof(f));
}/*
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezVec2& v)
{
Assert(m_MetaInfo.Type == ShaderVariableType::FLOAT_VEC2, "Variable type does not match!");
Set(&v, sizeof(v));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezVec3& v)
{
Assert(m_MetaInfo.Type == ShaderVariableType::FLOAT_VEC3, "Variable type does not match!");
Set(&v, sizeof(v));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezVec4& v)
{
Assert(m_MetaInfo.Type == ShaderVariableType::FLOAT_VEC4, "Variable type does not match!");
Set(&v, sizeof(v));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezMat3& m)
{
Assert(m_MetaInfo.Type == ShaderVariableType::FLOAT_MAT3, "Variable type does not match!");
Set(&m, sizeof(m));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezMat4& m)
{
Assert(m_MetaInfo.Type == ShaderVariableType::FLOAT_MAT4, "Variable type does not match!");
Set(&m, sizeof(m));
}*/
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(double f)
{
	Assert(m_MetaInfo.Type == ShaderVariableType::DOUBLE, "Variable type does not match!");
	Set(&f, sizeof(f));
}
/*template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezVec2d& v)
{
Assert(m_MetaInfo.Type == ShaderVariableType::DOUBLE_VEC2, "Variable type does not match!");
Set(&v, sizeof(v));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezVec3d& v)
{
Assert(m_MetaInfo.Type == ShaderVariableType::DOUBLE_VEC3, "Variable type does not match!");
Set(&v, sizeof(v));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezVec4d& v)
{
Assert(m_MetaInfo.Type == ShaderVariableType::DOUBLE_VEC4, "Variable type does not match!");
Set(&v, sizeof(v));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezMat3d& m)
{
Assert(m_MetaInfo.Type == ShaderVariableType::FLOAT_MAT3, "Variable type does not match!");
Set(&m, sizeof(m));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezMat4d& m)
{
Assert(m_MetaInfo.Type == ShaderVariableType::FLOAT_MAT4, "Variable type does not match!");
Set(&m, sizeof(m));
}*/
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(std::uint32_t ui)
{
	Assert(m_MetaInfo.Type == ShaderVariableType::UNSIGNED_INT, "Variable type does not match!");
	Set(&ui, sizeof(ui));
}
/*template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezVec2U32& v)
{
Assert(m_MetaInfo.Type == ShaderVariableType::UNSIGNED_INT_VEC2, "Variable type does not match!");
Set(&v, sizeof(v));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezVec3Template<std::uint32_t>& v)
{
Assert(m_MetaInfo.Type == ShaderVariableType::UNSIGNED_INT_VEC3, "Variable type does not match!");
Set(&v, sizeof(v));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezVec4Template<std::uint32_t>& v)
{
Assert(m_MetaInfo.Type == ShaderVariableType::UNSIGNED_INT_VEC4, "Variable type does not match!");
Set(&v, sizeof(v));
}
*/
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(std::int32_t i)
{
	Assert(m_MetaInfo.Type == ShaderVariableType::INT, "Variable type does not match!");
	Set(&i, sizeof(i));
}
/*template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezVec2I32& v)
{
Assert(m_MetaInfo.Type == ShaderVariableType::INT_VEC2, "Variable type does not match!");
Set(&v, sizeof(v));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezVec3Template<std::int32_t>& v)
{
Assert(m_MetaInfo.Type == ShaderVariableType::INT_VEC3, "Variable type does not match!");
Set(&v, sizeof(v));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezVec4Template<std::int32_t>& v)
{
Assert(m_MetaInfo.Type == ShaderVariableType::INT_VEC4, "Variable type does not match!");
Set(&v, sizeof(v));
}
*/
template<typename T>
T& StructuredBufferView::operator[](std::uint32_t _index)
{
    Assert( m_buffer->GetSize() >= (_index + 1) * sizeof(T),
        "Out-of-Bounds! The accessed struct does not fit into memory." );

    if( !m_mappedBuffer )
        m_mappedBuffer = m_buffer->Map();
    Assert( m_mappedBuffer, "The buffer object cannot be mapped. Wrong access specifiers?" );

    return *static_cast<T*>(m_mappedBuffer);
}

template<typename T>
const T& StructuredBufferView::operator[](std::uint32_t _index) const
{
    Assert( m_buffer->GetSize() >= (_index + 1) * sizeof(T),
        "Out-of-Bounds! The accessed struct does not fit into memory." );

    if( !m_mappedBuffer )
        m_mappedBuffer = m_buffer->Map();
    Assert( m_mappedBuffer, "The buffer object cannot be mapped. Wrong access specifiers?" );

    return *static_cast<const T*>(m_mappedBuffer);
}

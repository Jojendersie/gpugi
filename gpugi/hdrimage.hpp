#pragma once

#include <string>
#include <ei/matrix.hpp>
#include <cinttypes>

void WritePfm(const ei::Vec4* _data, const ei::IVec2& _size, const std::string& _filename, std::uint32_t _divisor);
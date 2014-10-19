#pragma once

#include <string>
#include <ei/matrix.hpp>

void WritePfm(const ei::Vec4* _data, const ei::IVec2& _size, const std::string& _filename);
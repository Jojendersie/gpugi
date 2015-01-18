#pragma once

#include <string>
#include <ei/matrix.hpp>
#include <cinttypes>

/// Writes HDR image to a rgb pfm.
///
/// Each color channel will be divided by the alpha channel!
/// \return true if successful, false otherwise.
bool WritePfm(const ei::Vec4* _data, const ei::IVec2& _size, const std::string& _filename, std::uint32_t _divisor);
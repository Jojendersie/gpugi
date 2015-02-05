#pragma once

#include <string>
#include <ei/vector.hpp>
#include <cinttypes>
#include <memory>

/// Writes HDR image to a rgb pfm.
///
/// Each color channel will be divided with the given divisor.
/// \return true if successful, false otherwise.
bool WritePfm(const ei::Vec4* _data, const ei::IVec2& _size, const std::string& _filename, std::uint32_t _divisor);

/// Loads HDR image from a rgb pfm.
std::unique_ptr<ei::Vec3[]> LoadPfm(const std::string& _filename, ei::IVec2& _size);
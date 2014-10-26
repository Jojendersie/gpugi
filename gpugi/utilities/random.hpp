#pragma once

#include <cinttypes>

std::uint32_t WangHash(std::uint32_t seed);

std::uint32_t Xorshift(std::uint32_t rndState);

std::uint64_t Xorshift(std::uint64_t rndState);
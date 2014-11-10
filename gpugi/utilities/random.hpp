#pragma once

#include <cinttypes>

std::uint32_t WangHash(std::uint32_t seed);

std::uint32_t Xorshift(std::uint32_t rndState);

std::uint64_t Xorshift(std::uint64_t rndState);
std::uint64_t Xorshift(std::uint64_t rndState, double& random01); // random01 ranges from 0 to 1 (exclusive)
std::uint64_t Xorshift(std::uint64_t rndState, float& random01); // random01 ranges from 0 to 1 (exclusive)
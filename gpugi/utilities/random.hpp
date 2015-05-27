#pragma once

#include <cinttypes>

std::uint32_t WangHash(std::uint32_t seed);

std::uint32_t Xorshift(std::uint32_t rndState);

std::uint64_t Xorshift(std::uint64_t rndState);
double XorshiftD(std::uint64_t& rndState); // random01 ranges from 0 to 1 (exclusive)
float XorshiftF(std::uint64_t& rndState); // random01 ranges from 0 to 1 (exclusive)

// random number in [_min, _max)
float Xorshift(std::uint64_t& _rndState, float _min, float _max);
#include "random.hpp"

// http://www.reedbeta.com/blog/2013/01/12/quick-and-easy-gpu-random-numbers-in-d3d11/

std::uint32_t WangHash(std::uint32_t seed)
{
	seed = (seed ^ 61) ^ (seed >> 16);
	seed *= 9;
	seed = seed ^ (seed >> 4);
	seed *= 0x27d4eb2d;
	seed = seed ^ (seed >> 15);
	return seed;
}

std::uint32_t Xorshift(std::uint32_t rndState)
{
	// Xorshift algorithm from George Marsaglia's paper
	rndState ^= (rndState << 13);
	rndState ^= (rndState >> 17);
	rndState ^= (rndState << 5);
	return rndState;
}

// https://en.wikipedia.org/wiki/Xorshift
std::uint64_t Xorshift(std::uint64_t rndState)
{
	rndState ^= rndState >> 12;
	rndState ^= rndState << 25;
	rndState ^= rndState >> 27;
	return rndState * UINT64_C(2685821657736338717);
}

std::uint64_t Xorshift(std::uint64_t rndState, double& random01)
{
	rndState = Xorshift(rndState);
	random01 = static_cast<double>(rndState % 900719925474099) / 900719925474099.0; // 2^53 (52bit mantissa!)
	return rndState;
}

std::uint64_t Xorshift(std::uint64_t rndState, float& random01)
{
	rndState = Xorshift(rndState);
	random01 = static_cast<float>(rndState % 8388593) / 8388593.0f;
	return rndState;
}
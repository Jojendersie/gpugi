#pragma once

enum Result
{
	FAILURE = 0,
	SUCCEEDED = 1
};

// Delete and set to nullptr
#define SAFE_DELETE(x) do { delete x; x = nullptr; } while (false)

template<typename T>
T Clamp(T v, T min, T max)
{
	if (v < min)
		return min;
	else if (v > max)
		return max;
	else
		return v;
}
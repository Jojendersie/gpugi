#pragma once

enum Result
{
	FAILURE = 0,
	SUCCEEDED = 1
};

// Delete and set to nullptr
#define SAFE_DELETE(x) do { delete x; x = nullptr; } while (false)
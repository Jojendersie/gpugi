// Use this function to get an initial random seed.
uint InitRandomSeed(uint frameSeed, uint invocationID)
{
	uint seed = frameSeed + invocationID;

	// Wang hash.
	seed = (seed ^ 61) ^ (seed >> 16);
	seed *= 9;
	seed = seed ^ (seed >> 4);
	seed *= 0x27d4eb2d;
	seed = seed ^ (seed >> 15);
	return seed;
}

float Random(inout uint seed)
{
	// Xorshift32
	seed ^= (seed << 13);
	seed ^= (seed >> 17);
	seed ^= (seed << 5);
	
	return float(seed % 8388608) / 8388608.0;
}

vec2 Random2(inout uint seed)
{
	return vec2(Random(seed), Random(seed));
}
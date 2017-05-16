uint wanghash(uint x)
{
	x = (x ^ 61) ^ (x >> 16);
	x *= 9;
	x = x ^ (x >> 4);
	x *= 0x27d4eb2d;
	x = x ^ (x >> 15);
	return x;
}

// Use this function to get an initial random seed in eye-paths or not at all.
// It introduces structured noise and leads to artifacts in all light-tracing paths.
uint InitCoherentRandomSeed(uint frameSeed, uint invocationID)
{
	uint seed = frameSeed;// + wanghash(invocationID) % 5;

	return wanghash(seed);
}
// Use this standard with another rng sequence per sample
uint InitRandomSeed(uint frameSeed, uint invocationID)
{
	uint seed = frameSeed + invocationID;

	return wanghash(seed);
}

uint RandomUInt(inout uint seed)
{
	// Xorshift32
	seed ^= (seed << 13);
	seed ^= (seed >> 17);
	seed ^= (seed << 5);

	return seed;
}

float Random(inout uint seed)
{
	return float(RandomUInt(seed) % 8388593) / 8388593.0;
}


vec2 Random2(inout uint seed)
{
	return vec2(Random(seed), Random(seed));
}

// Sample a uniform direction
vec3 SampleDirection(vec2 randomSample)
{
	float cosTheta = randomSample.x * 2.0f - 1.0f;
	float sinTheta = sqrt((1.0f - cosTheta) * (1.0f + cosTheta));
	float phi = randomSample.y * PI_2;
	return vec3(sinTheta * sin(phi), sinTheta * cos(phi), cosTheta);
}

// Sample hemisphere with cosine density.
//    randomSample is a random number between 0-1
vec3 SampleHemisphereCosine(vec2 randomSample, vec3 U, vec3 V, vec3 W)
{
	float phi = PI_2 * randomSample.x;
	float sinTheta = sqrt(randomSample.y);	// sin(acos(sqrt(1-x))) = sqrt(x)
	float x = sinTheta * cos(phi);
	float y = sinTheta * sin(phi);
	float z = sqrt(1.0 - randomSample.y);	// sqrt(1-sin(theta)^2)

    return x*U + y*V + z*W;
}


// Sample Phong lobe relative to U, V, W frame
vec3 SamplePhongLobe(vec2 randomSample, float exponent, vec3 U, vec3 V, vec3 W)
{
	float phi = PI_2 * randomSample.x;
	//float power = exp(log(randomSample.y) / (exponent+1.0)); // equivalent but slower in AMD Shader analyser
	float power = pow(randomSample.y, 1.0 / (exponent+1.0));
	float scale = sqrt(1.0 - power*power);

	float x = cos(phi)*scale;
	float y = sin(phi)*scale;
	float z = power;

	return x*U + y*V + z*W;
}

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
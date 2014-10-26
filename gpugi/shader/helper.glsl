#define PI 3.14159265358979
#define PI_2 6.28318530717958

float saturate(float x)
{
	return clamp(x, 0.0, 1.0);
}
vec2 saturate(vec2 x)
{
	return clamp(x, vec2(0.0), vec2(1.0));
}
vec3 saturate(vec3 x)
{
	return clamp(x, vec3(0.0), vec3(1.0));
}
vec4 saturate(vec4 x)
{
	return clamp(x, vec4(0.0), vec4(1.0));
}

// Sample hemisphere with cosine density.
//    randomSample is a random number between 0-1
vec3 SampleUnitHemisphere(vec2 randomSample, vec3 U, vec3 V, vec3 W)
{
    float phi = PI_2 * randomSample.x;
    float r = sqrt(randomSample.y);
    float x = r * cos(phi);
    float y = r * sin(phi);
    //float z = sqrt(saturate(1.0 - x*x - y*y));
	float z = 1.0 - x*x -y*y;
    z = z > 0.0 ? sqrt(z) : 0.0;

    return x*U + y*V + z*W;
}

// Create ONB from normalized vector
void CreateONB(in vec3 n, out vec3 U, out vec3 V)
{
	U = cross(n, vec3(0.0, 1.0, 0.0));
	if (abs(U.x) < 0.001f && abs(U.y) < 0.001f && abs(U.z) < 0.001f)
		U = cross(n, vec3(1.0, 0.0, 0.0));
	U = normalize(U);
	V = cross(n, U);
}

// Sample Phong lobe relative to U, V, W frame
vec3 SamplePhongLobe(vec2 randomSample, float exponent, vec3 U, vec3 V, vec3 W)
{
	float power = exp(log(randomSample.y) / (exponent+1.0f));
	float phi = randomSample.x * 2.0 * PI;
	float scale = sqrt(1.0 - power*power);

	float x = cos(phi)*scale;
	float y = sin(phi)*scale;
	float z = power;

	return x*U + y*V + z*W;
}
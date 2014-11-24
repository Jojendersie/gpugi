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

// Create ONB from normalized vector
void CreateONB(in vec3 n, out vec3 U, out vec3 V)
{
	U = cross(n, vec3(0.0, 1.0, 0.0));
	if (all(lessThan(abs(U), vec3(0.0001))))
		U = cross(n, vec3(1.0, 0.0, 0.0));
	U = normalize(U);
	V = cross(n, U);
}

float GetLuminance(vec3 rgb)
{
	const vec3 W = vec3(0.2125, 0.7154, 0.0721);
	return dot(rgb, W);
}

// Expects: vec2(atan(normal.y, normal.x), normal.z)
vec3 UnpackNormal(in vec2 packedNormal)
{
	float sinPhi = sqrt(1.0 - packedNormal.y*packedNormal.y);
	return vec3(cos(packedNormal.x)*sinPhi, sin(packedNormal.x)*sinPhi, packedNormal.y);
}
// Returns: vec2(atan(normal.y, normal.x), normal.z)
// Attention: return.x ranges [-PI;+PI]
vec2 PackNormal(in vec3 normal)
{
	return vec2(atan(normal.x, normal.y), normal.z);
}


/*
// Source http://www.malteclasen.de/zib/index4837.html?p=37
uint SharedExponentEncode(vec3 value)
{
	value = value / 65536.0;
	vec3 exponent = clamp(ceil(log2(value)), -128.0, 127.0);
	float commonExponent = max(max(exponent.r, exponent.g), exponent.b);
	float range = exp2(commonExponent);
	vec3 mantissa = clamp(value / range, 0.0, 1.0);
	return packUnorm4x8(vec4(mantissa, (commonExponent + 128.0) / 256.0));
}
vec3 SharedExponentDecode(uint encoded)
{
	vec4 encodedUnpacked = unpackUnorm4x8(encoded);
	float exponent = encodedUnpacked.a * 256.0 - 128.0;
	return (exp2(exponent) * 65536.0) * encodedUnpacked.rgb;
}
*/
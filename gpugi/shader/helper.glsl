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
	const vec3 W = vec3(0.212671, 0.715160, 0.072169);
	return dot(rgb, W);
}

// Expects: vec2(atan(normal.y, normal.x), normal.z)
vec3 UnpackNormal(in vec2 packedNormal)
{
	float sinPhi = sqrt(saturate((1.0 - packedNormal.y) * (1.0 + packedNormal.y)));
	return vec3(cos(packedNormal.x)*sinPhi, sin(packedNormal.x)*sinPhi, packedNormal.y);
}
vec3 UnpackNormal(float packedNormal0, float packedNormal1)
{
	float sinPhi = sqrt(1.0 - packedNormal1*packedNormal1);
	return vec3(cos(packedNormal0)*sinPhi, sin(packedNormal0)*sinPhi, packedNormal1);
}

// Returns: vec2(atan(normal.y, normal.x), normal.z)
// Attention: return.x ranges [-PI;+PI]
vec2 PackNormal(in vec3 normal)
{
	// Currently invalid implementation which tries to remove division by 0 with an epsilon.
	//const float ATAN_EPS = 0.0001;
	//return vec2(atan(normal.y, step(abs(normal.x), ATAN_EPS) * ATAN_EPS + normal.x), normal.z);
	// A mathematical correct and more robust alternative would be using 2 atan functions:
	// http://stackoverflow.com/questions/26070410/robust-atany-x-on-glsl-for-converting-xy-coordinate-to-angle
	//return vec2(PI/2 - mix(atan(normal.x, normal.y), atan(normal.y, normal.x), abs(normal.x) > abs(normal.y)), normal.z);
	// Alternative fix (selfmade):
	// When x is zero then cos(packedNormal0)==0. So packedNormal0 may be +-PI/2
	// dependent on the sign of y.
	return vec2(normal.x == 0.0 ? sign(normal.y)*PI/2 : atan(normal.y, normal.x), normal.z);
}

// Pack a direction into 32 bit.
// Returns packSnorm2x16(atan(normal.y, normal.x) / PI, normal.z)
uint PackNormal32(in vec3 normal)
{
	return packSnorm2x16(vec2(normal.x == 0.0 ? sign(normal.y)/2 : atan(normal.y, normal.x)/PI, normal.z));
}
vec3 UnpackNormal(uint packedNormal)
{
	vec2 packedVec = unpackSnorm2x16(packedNormal);
	packedVec.x *= PI;
	return UnpackNormal(packedVec);
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

// Expects: vec3([0,1], [0,1], [0,1])
// Returns discretization of the value
uint PackR11G11B10(vec3 color)
{
	color *= vec3(2047.0, 2047.0, 1023.0);
	return (uint(color.r) << 21) | (uint(color.g) << 10) | uint(color.b);
}

vec3 UnpackR11G11B10(uint code)
{
	vec3 color;
	color.r = float(code >> 21) / 2047.0;
	color.g = float((code >> 10) & 0x7ff) / 2047.0;
	color.b = float(code & 0x3ff) / 1023.0;
	return color;
}

// YCgCo Color space conversions
vec3 rgbToYCgCo(vec3 _rgb)
{
	return vec3(dot(_rgb, vec3(0.25, 0.5, 0.25)),
				dot(_rgb, vec3(-0.25, 0.5, -0.25)),
				dot(_rgb.xz, vec2(0.5, -0.5)));
}

vec3 yCgCoToRGB(vec3 _yCgCo)
{
	float tmp = _yCgCo.x - _yCgCo.y;
	return vec3(tmp + _yCgCo.z, _yCgCo.x + _yCgCo.y, tmp - _yCgCo.z);
}

// Pack an HDR color [0,inf]^3 into 64 bits.
// Converts the RGB color to YCgCo, stores Y as float and CgCo as
// signed shared exponent values. SCg12_SCo12S_E8
vec2 PackHDR64(vec3 _color)
{
	_color = rgbToYCgCo(_color);
	// Pack CoCg
	float maxValue = max(abs(_color.y), abs(_color.z));
	float commonExponent = clamp(ceil(log2(maxValue)), -126.0, 127.0);
	float range = exp2(commonExponent);
	vec2 mantissa = clamp(_color.yz * 2048.0 / range + 2048.0, 0.0, 4095.0);
	return vec2(_color.x, uintBitsToFloat(
		(uint(mantissa.x) << 20u) | (uint(mantissa.y) << 8u) | uint(commonExponent + 126.0)
	));
}

vec3 UnpackHDR64(vec2 _hdr)
{
	vec3 color;
	color.x	= _hdr.x;
	uint packCgCo = floatBitsToUint(_hdr.y);
	float exponent = float(packCgCo & 0xff) - 126.0;
	float range = exp2(exponent);
	float scale = range / 2048.0;
	color.y = (float(packCgCo >> 20u) - 2048.0) * scale;
	color.z = (float((packCgCo >> 8u) & 0xfff) - 2048.0) * scale;
	return yCgCoToRGB(color);
}
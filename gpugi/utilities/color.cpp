#include "color.hpp"
#include "utils.hpp"
#include <cmath>
#include <algorithm>

namespace ColorUtils
{
	std::uint32_t SharedExponentEncode(const ei::Vec3& value)
	{
		ei::Vec3 valueScaled = value / 65536.0f;
		ei::Vec3 exponent = ei::Vec3(Clamp(ceil(log2(valueScaled.r)), -128.0f, 127.0f),
			Clamp(ceil(log2(valueScaled.g)), -128.0f, 127.0f),
			Clamp(ceil(log2(valueScaled.b)), -128.0f, 127.0f));


		float commonExponent = std::max(std::max(exponent.r, exponent.g), exponent.b);
		float range = exp2(commonExponent);
		ei::Vec3 mantissa = ei::Vec3(Clamp(valueScaled.x / range, 0.0f, 1.0f),
			Clamp(valueScaled.y / range, 0.0f, 1.0f),
			Clamp(valueScaled.z / range, 0.0f, 1.0f));

		return EncodeRGBA8(ei::Vec4(mantissa.x, mantissa.y, mantissa.z, (commonExponent + 128.0f) / 256.0f)).rgba; // some redundant multiplication...
	}

	ei::Vec3 SharedExponentDecode(std::uint32_t encoded)
	{
		ei::Vec4 encodedColor = DecodeRGBA8(*reinterpret_cast<ByteColor*>(&encoded));
		float exponent = encodedColor.a * 256.0f - 128.0f;
		ei::Vec3 mantissa(encodedColor.r, encodedColor.g, encodedColor.b);
		return exp2(exponent) * 65536.0f * mantissa;
	}

	ei::Vec4 DecodeRGBA8(ByteColor color)
	{
		return ei::Vec4(color.r * 255.0f, color.g * 255.0f, color.b * 255.0f, color.a * 255.0f);
	}

	ByteColor EncodeRGBA8(const ei::Vec4& color)
	{
		ByteColor out;
		out.r = static_cast<std::uint8_t>(color.r * 255);
		out.g = static_cast<std::uint8_t>(color.g * 255);
		out.b = static_cast<std::uint8_t>(color.b * 255);
		out.a = static_cast<std::uint8_t>(color.a * 255);
		return out;
	}
}
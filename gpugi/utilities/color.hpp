#pragma once

#include <ei/matrix.hpp>
#include <cinttypes>

namespace ColorUtils
{
	union ByteColor
	{
		std::uint32_t rgba;
		struct
		{
			std::uint8_t r, g, b, a;
		};
	};

	ei::Vec4 DecodeRGBA8(ByteColor color);

	ByteColor EncodeRGBA8(const ei::Vec4& color);


	std::uint32_t SharedExponentEncode(const ei::Vec3& value);

	ei::Vec3 SharedExponentDecode(std::uint32_t color);
}
#include "hdrimage.hpp"
#include "utilities\logger.hpp"
#include <fstream>


void WritePfm(const ei::Vec4* _data, const ei::IVec2& _size, const std::string& _filename, std::uint32_t _divisor)
{
	std::ofstream file(_filename.c_str(), std::ios::binary);
	if(!file.bad() && !file.fail())
	{
		file.write("PF\n", sizeof(char) * 3);
		file << _size.x << " " << _size.y << "\n";

		file.write("-1.000000\n", sizeof(char) * 10);

		const ei::Vec4* v = _data;
		for (int y = 0; y < _size.y; ++y)
		{
			for (int x = 0; x < _size.x; ++x)
			{
				ei::Vec3 outV(v->x / _divisor, v->y / _divisor, v->z / _divisor);
				file.write(reinterpret_cast<const char*>(&outV), sizeof(float) * 3);
				++v;
			}
		}
	}
	else
	{
		LOG_ERROR("Error writing hdr image to " + _filename);
	}
}

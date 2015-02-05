#include "hdrimage.hpp"
#include "utilities\logger.hpp"
#include <fstream>


bool WritePfm(const ei::Vec4* _data, const ei::IVec2& _size, const std::string& _filename, std::uint32_t _divisor)
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
		return true;
	}
	else
	{
		LOG_ERROR("Error writing hdr image to " + _filename);
		return false;
	}
}

std::unique_ptr<ei::Vec3[]> LoadPfm(const std::string& _filename, ei::IVec2& _size)
{
	std::ifstream file(_filename.c_str(), std::ios::binary);
	if (!file.bad() && !file.fail())
	{
		char header[4];
		file.read(header, sizeof(char) * 3);
		header[3] = '\0';
		if (strcmp(header, "PF\n") != 0)
			return nullptr;

		file >> _size.x >> _size.y;
		std::unique_ptr<ei::Vec3[]> out(new ei::Vec3[_size.x * _size.y]);

		file.ignore(11);
		
		ei::Vec3* v = out.get();
		for (int y = 0; y < _size.y; ++y)
		{
			for (int x = 0; x < _size.x; ++x)
			{
				file.read(reinterpret_cast<char*>(v), sizeof(float) * 3);
				++v;
			}
		}
		return out;
	}
	else
	{
		LOG_ERROR("Error reading hdr image from " + _filename);
		return nullptr;
	}
}
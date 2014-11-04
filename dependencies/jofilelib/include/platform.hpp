#pragma once

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__)
#	define JO_WINDOWS
#elif defined(__APPLE__)
#	define JO_IOS
#elif defined(unix) || defined(__unix__) || defined(__unix)
#	define JO_UNIX
#elif defined(__posix)
#	define JO_POSIX
#endif

#include <cstdint>

namespace Jo {

	/// \brief Check the native endianness of the current system.
	inline bool IsLittleEndian()
	{
		static const uint16_t EndianCheck(0x00ff);
		return ( *((uint8_t*)&EndianCheck) == 0xff);
	}

	/// \brief A class template to enable partial specialization for the types
	///		size.
	template<typename T, int Size> struct EndianConverter { static T Convert( T i ); };
	template<typename T> struct EndianConverter<T,2> {
		static T Convert( T i )	{
			uint16_t r = *reinterpret_cast<uint16_t*>(&i);
			r = r>>8 | r<<8;
			return *reinterpret_cast<T*>(&r);
		} 
	};
	template<typename T> struct EndianConverter<T,4> {
		static T Convert( T i )	{
			uint32_t r = *reinterpret_cast<uint32_t*>(&i);
			r = (r&0xff00ff00)>>8 | (r&0x00ff00ff)<<8;
			r = r>>16 | r<<16;
			return *reinterpret_cast<T*>(&r);
		} 
	};
	template<typename T> struct EndianConverter<T,8> {
		static T Convert( T i )	{
			uint64_t r = *reinterpret_cast<uint64_t*>(&i);
			r = (r&0xff00ff00ff00ff00)>>8 | (r&0x00ff00ff00ff00ff)<<8;
			r = (r&0xffff0000ffff0000)>>16 | (r&0x0000ffff0000ffff)<<16;
			r = r>>32 | r<<32;
			return *reinterpret_cast<T*>(&r);
		}
	};

	/// \brief Convert the endianness of an arbitrary type with 2,4 or 8 bytes.
	/// \details The type is deduced automatically.
	template<typename T> T ConvertEndian( T i )	{ return EndianConverter<T,sizeof(T)>::Convert(i); }

	/// \brief Make sure the endianness of an arbitrary type with 2,4 or 8 bytes is little.
	/// \details The type is deduced automatically.
	template<typename T> T AsLittleEndian( T i )	{ if(IsLittleEndian()) return i; else return EndianConverter<T,sizeof(T)>::Convert(i); }

	/// \brief Make sure the endianness of an arbitrary type with 2,4 or 8 bytes is big.
	/// \details The type is deduced automatically.
	template<typename T> T AsBigEndian( T i )	{ if(IsLittleEndian()) return EndianConverter<T,sizeof(T)>::Convert(i); else return i; }
} // namespace Jo
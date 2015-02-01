// Shortened config file - requires asserts for pathutils

#include "../gpugi/utilities/logger.hpp"
#include "../gpugi/utilities/assert.hpp"



// Assert
#ifdef _DEBUG
#define GLHELPER_ASSERT(condition, message) Assert(condition, message)
#else
#define GLHELPER_ASSERT(condition, string) do { } while(false)
#endif



// Logging
#define GLHELPER_LOG_ERROR(message)		LOG_ERROR(message)
#define GLHELPER_LOG_WARNING(message)	LOG_ERROR(message)
#define GLHELPER_LOG_INFO(message)		LOG_ERROR(message)



// Vector & Matrix types.
#include <ei/vector.hpp>
namespace gl
{
	typedef ei::Vec2 Vec2;
	typedef ei::Vec3 Vec3;
	typedef ei::Vec4 Vec4;

	typedef ei::IVec2 IVec2;
	typedef ei::IVec3 IVec3;
	typedef ei::IVec4 IVec4;

	typedef ei::UVec2 UVec2;
	typedef ei::UVec3 UVec3;
	typedef ei::UVec4 UVec4;

	typedef ei::Mat3x3 Mat3;
	typedef ei::Mat4x4 Mat4;
};
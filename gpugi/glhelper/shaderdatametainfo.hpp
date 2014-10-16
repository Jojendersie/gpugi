#pragma once

#include <GL/GL.h>
#include <cstdint>
#include <unordered_map>

#include "../utilities/assert.hpp"

namespace gl
{
	enum class ShaderVariableType
	{
		FLOAT = GL_FLOAT,
		FLOAT_VEC2 = GL_FLOAT_VEC2,
		FLOAT_VEC3 = GL_FLOAT_VEC3,
		FLOAT_VEC4 = GL_FLOAT_VEC4,

		DOUBLE = GL_DOUBLE,
		DOUBLE_VEC2 = GL_DOUBLE_VEC2,
		DOUBLE_VEC3 = GL_DOUBLE_VEC3,
		DOUBLE_VEC4 = GL_DOUBLE_VEC4,

		INT = GL_INT,
		INT_VEC2 = GL_INT_VEC2,
		INT_VEC3 = GL_INT_VEC3,
		INT_VEC4 = GL_INT_VEC4,

		UNSIGNED_INT = GL_UNSIGNED_INT,
		UNSIGNED_INT_VEC2 = GL_UNSIGNED_INT_VEC2,
		UNSIGNED_INT_VEC3 = GL_UNSIGNED_INT_VEC3,
		UNSIGNED_INT_VEC4 = GL_UNSIGNED_INT_VEC4,

		BOOL = GL_BOOL,
		BOOL_VEC2 = GL_BOOL_VEC2,
		BOOL_VEC3 = GL_BOOL_VEC3,
		BOOL_VEC4 = GL_BOOL_VEC4,

		FLOAT_MAT​2 = GL_FLOAT_MAT2,
		FLOAT_MAT3 = GL_FLOAT_MAT3,
		FLOAT_MAT4 = GL_FLOAT_MAT4,
		FLOAT_MAT2x3​ = GL_FLOAT_MAT2x3,
		FLOAT_MAT2x4 = GL_FLOAT_MAT2x4,
		FLOAT_MAT3x2 = GL_FLOAT_MAT3x2,
		FLOAT_MAT3x4 = GL_FLOAT_MAT3x4,
		FLOAT_MAT4x2 = GL_FLOAT_MAT4x2,
		FLOAT_MAT4x3 = GL_FLOAT_MAT4x3,

		DOUBLE_MAT2 = GL_DOUBLE_MAT2,
		DOUBLE_MAT3 = GL_DOUBLE_MAT3,
		DOUBLE_MAT4 = GL_DOUBLE_MAT4,
		DOUBLE_MAT2x3​ = GL_DOUBLE_MAT2x3,
		DOUBLE_MAT2x4 = GL_DOUBLE_MAT2x4,
		DOUBLE_MAT3x2 = GL_DOUBLE_MAT3x2,
		DOUBLE_MAT3x4 = GL_DOUBLE_MAT3x4,
		DOUBLE_MAT4x2 = GL_DOUBLE_MAT4x2,
		DOUBLE_MAT4x3 = GL_DOUBLE_MAT4x3,

		SAMPLER_1D = GL_SAMPLER_1D,
		SAMPLER_2D = GL_SAMPLER_2D,
		SAMPLER_3D = GL_SAMPLER_3D,
		SAMPLER_CUBE​ = GL_SAMPLER_CUBE,
		SAMPLER_1D_SHADOW = GL_SAMPLER_1D_SHADOW,
		SAMPLER_2D_SHADOW = GL_SAMPLER_2D_SHADOW,
		SAMPLER_1D_ARRAY = GL_SAMPLER_1D_ARRAY,
		SAMPLER_2D_ARRAY = GL_SAMPLER_2D_ARRAY,
		SAMPLER_1D_ARRAY_SHADOW = GL_SAMPLER_1D_ARRAY_SHADOW,
		SAMPLER_2D_ARRAY_SHADOW = GL_SAMPLER_2D_ARRAY_SHADOW,
		SAMPLER_2D_MULTISAMPLE = GL_SAMPLER_2D_MULTISAMPLE,
		SAMPLER_2D_MULTISAMPLE_ARRAY = GL_SAMPLER_2D_MULTISAMPLE_ARRAY,
		SAMPLER_CUBE_SHADOW = GL_SAMPLER_CUBE_SHADOW,
		SAMPLER_BUFFER = GL_SAMPLER_BUFFER,
		SAMPLER_2D_RECT = GL_SAMPLER_2D_RECT,
		SAMPLER_2D_RECT_SHADOW = GL_SAMPLER_2D_RECT_SHADOW,
		INT_SAMPLER_1D = GL_INT_SAMPLER_1D,
		INT_SAMPLER_2D = GL_INT_SAMPLER_2D,
		INT_SAMPLER_3D = GL_INT_SAMPLER_3D,
		INT_SAMPLER_CUBE = GL_INT_SAMPLER_CUBE,
		INT_SAMPLER_1D_ARRAY = GL_INT_SAMPLER_1D_ARRAY,
		INT_SAMPLER_2D_ARRAY = GL_INT_SAMPLER_2D_ARRAY,
		INT_SAMPLER_2D_MULTISAMPLE = GL_INT_SAMPLER_2D_MULTISAMPLE,
		INT_SAMPLER_2D_MULTISAMPLE_ARRAY = GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY,
		INT_SAMPLER_BUFFER = GL_INT_SAMPLER_BUFFER,
		INT_SAMPLER_2D_RECT = GL_INT_SAMPLER_2D_RECT,
		UNSIGNED_INT_SAMPLER_1D = GL_UNSIGNED_INT_SAMPLER_1D,
		UNSIGNED_INT_SAMPLER_2D = GL_UNSIGNED_INT_SAMPLER_2D,
		UNSIGNED_INT_SAMPLER_3D = GL_UNSIGNED_INT_SAMPLER_3D,
		UNSIGNED_INT_SAMPLER_CUBE​ = GL_UNSIGNED_INT_SAMPLER_CUBE,
		UNSIGNED_INT_SAMPLER_1D_ARRAY = GL_UNSIGNED_INT_SAMPLER_1D_ARRAY,
		UNSIGNED_INT_SAMPLER_2D_ARRAY = GL_UNSIGNED_INT_SAMPLER_2D_ARRAY,
		UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE = GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE,
		UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY = GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY,
		UNSIGNED_INT_SAMPLER_BUFFER = GL_UNSIGNED_INT_SAMPLER_BUFFER,
		UNSIGNED_INT_SAMPLER_2D_RECT = GL_UNSIGNED_INT_SAMPLER_2D_RECT,

		IMAGE_1D = GL_IMAGE_1D,
		IMAGE_2D = GL_IMAGE_2D,
		IMAGE_3D = GL_IMAGE_3D,
		IMAGE_2D_RECT = GL_IMAGE_2D_RECT,
		IMAGE_CUBE = GL_IMAGE_CUBE,
		IMAGE_BUFFER = GL_IMAGE_BUFFER,
		IMAGE_1D_ARRAY = GL_IMAGE_1D_ARRAY,
		IMAGE_2D_ARRAY = GL_IMAGE_2D_ARRAY,
		IMAGE_2D_MULTISAMPLE = GL_IMAGE_2D_MULTISAMPLE,
		IMAGE_2D_MULTISAMPLE_ARRAY = GL_IMAGE_2D_MULTISAMPLE_ARRAY,
		INT_IMAGE_1D = GL_INT_IMAGE_1D,
		INT_IMAGE_2D = GL_INT_IMAGE_2D,
		INT_IMAGE_3D = GL_INT_IMAGE_3D,
		INT_IMAGE_2D_RECT = GL_INT_IMAGE_2D_RECT,
		INT_IMAGE_CUBE = GL_INT_IMAGE_CUBE,
		INT_IMAGE_BUFFER = GL_INT_IMAGE_BUFFER,
		INT_IMAGE_1D_ARRAY = GL_INT_IMAGE_1D_ARRAY,
		INT_IMAGE_2D_ARRAY = GL_INT_IMAGE_2D_ARRAY,
		INT_IMAGE_2D_MULTISAMPLE = GL_INT_IMAGE_2D_MULTISAMPLE,
		INT_IMAGE_2D_MULTISAMPLE_ARRAY = GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY,
		UNSIGNED_INT_IMAGE_1D = GL_UNSIGNED_INT_IMAGE_1D,
		UNSIGNED_INT_IMAGE_2D = GL_UNSIGNED_INT_IMAGE_2D,
		UNSIGNED_INT_IMAGE_3D = GL_UNSIGNED_INT_IMAGE_3D,
		UNSIGNED_INT_IMAGE_2D_RECT = GL_UNSIGNED_INT_IMAGE_2D_RECT,
		UNSIGNED_INT_IMAGE_CUBE = GL_UNSIGNED_INT_IMAGE_CUBE,
		UNSIGNED_INT_IMAGE_BUFFER = GL_UNSIGNED_INT_IMAGE_BUFFER,
		UNSIGNED_INT_IMAGE_1D_ARRAY = GL_UNSIGNED_INT_IMAGE_1D_ARRAY,
		UNSIGNED_INT_IMAGE_2D_ARRAY = GL_UNSIGNED_INT_IMAGE_2D_ARRAY,
		UNSIGNED_INT_IMAGE_2D_MULTISAMPLE = GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE,
		UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY = GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY,

		UNSIGNED_INT_ATOMIC_COUNTER = GL_UNSIGNED_INT_ATOMIC_COUNTER
	};

	/// Basic information block for Buffer information
	template<typename VariableType>
	struct BufferInfo
	{
		GLint     iBufferBinding;       ///< OpengGL buffer binding index
		std::int32_t   iBufferDataSizeByte;  ///< Minimal buffer size in bytes

		/// Known contained variable information
		std::unordered_map<std::string, VariableType> Variables;

		// possible, but currently missing:
		// - usage by shader stage (GL_REFERENCED_BY_..)

	private:
		friend class ShaderObject;

		// internal buffer index for connection variables with buffers
		std::int32_t   iInternalBufferIndex;
	};

	/// Basic information block for all possible shader variables
	struct ShaderVariableInfoBase
	{
		ShaderVariableType  Type;         ///< Data type
		std::int32_t		iBlockOffset; ///< Offset in corresponding buffer, -1 if there's no buffer.

		std::int32_t iArrayElementCount;   ///< Number of array elements.
		std::int32_t iArrayStride; ///< Stride between array elements in bytes.

		std::int32_t iMatrixStride;  ///< Stride between columns of a column-major matrix or rows of a row-major matrix.
		bool    bRowMajor;

		// possible, but currently missing:
		// - usage by shader stage (GL_REFERENCED_BY_..)
	};
	/// Information block for uniform variables
	struct UniformVariableInfo : ShaderVariableInfoBase
	{
		GLint iLocation;      ///< OpenGL location, -1 if used in a buffer.
		GLint iAtomicCounterbufferIndex; ///< Index of active atomic counter buffer containing this variable​. -1 if this is no atomic counter buffer
	};
	/// Info block for buffer variables
	struct BufferVariableInfo : ShaderVariableInfoBase
	{
		std::int32_t iTopLevelArraySize;
		std::int32_t iTopLevelArrayStride;
	};

	typedef BufferInfo<BufferVariableInfo> ShaderStorageBufferMetaInfo;
	typedef BufferInfo<UniformVariableInfo> UniformBufferMetaInfo;

	/// \brief Base class for setable variable - basically a wrapper for
	template<typename VariableType>
	class ShaderVariable
	{
	public:
		void Set(float f);
		/* void Set(const ezVec2& v);
		 void Set(const ezVec3& v);
		 void Set(const ezVec4& v);
		 void Set(const ezMat3& m);
		 void Set(const ezMat4& m);*/

		void Set(double f);
		/*    void Set(const ezVec2d& v);
			void Set(const ezVec3d& v);
			void Set(const ezVec4d& v);
			void Set(const ezMat3d& m);
			void Set(const ezMat4d& m); */

		void Set(std::uint32_t ui);
		/* void Set(const ezVec2U32& v);
		 void Set(const ezVec3Template<std::int32_t>& v);
		 void Set(const ezVec4Template<std::int32_t>& v);*/
		void Set(std::int32_t i);
		/*void Set(const ezVec2I32& v);
		void Set(const ezVec3Template<std::uint32_t>& v);
		void Set(const ezVec4Template<std::uint32_t>& v);*/

		virtual void Set(const void* pData, std::uint32_t uiSizeInBytes) = 0;

		// add more type implementations here if necessary

		const VariableType& GetMetaInfo() const        { return m_MetaInfo; }

	protected:
		ShaderVariable() {}
		ShaderVariable(const VariableType& metaInfo) : m_MetaInfo(metaInfo) {}

		VariableType m_MetaInfo;
	};

#include "ShaderDataMetaInfo.inl"
}
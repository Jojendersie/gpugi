#pragma once

#include "texture.hpp"
#include <memory>
#include <string>

namespace gl
{
	class Texture2D : public Texture
	{
	public:
		/// \param uiNumMipLevels   -1 for full chain, 0 and 1 have same result
		Texture2D(std::uint32_t width, std::uint32_t height, TextureFormat format, std::int32_t numMipLevels = 1, std::uint32_t numMSAASamples = 0);

		/// \brief Loads texture from file using stb_image
		///
		/// Uses given to create an object if file loading is successful. Use corresponding deallocator to remove the texture.
		/// Will return NULL on error.
		static std::unique_ptr<Texture2D> LoadFromFile(const std::string& _filename, bool _generateMipMaps = true, bool _sRGB = false);

		/// Overwrites data of a given mip level.
		void SetData(std::uint32_t _mipLevel, TextureSetDataFormat _dataFormat, TextureSetDataType _dataType, const void* _pData);

		void GenMipMaps();

		GLenum GetOpenGLTextureType() override { return GetNumMSAASamples() > 0 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D; }
	};

}


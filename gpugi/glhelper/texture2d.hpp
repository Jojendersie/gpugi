#pragma once

#include "texture.hpp"

namespace gl
{
	class Texture2D : public Texture
	{
	public:
		/// \param uiNumMipLevels   -1 for full chain, 0 and 1 have same result
		Texture2D(std::uint32_t width, std::uint32_t height, TextureFormat format, std::int32_t numMipLevels = 1, std::uint32_t numMSAASamples = 0);

		// TODO: Not reimplemented yet
		/// \brief Loads texture from file using stb_image
		///
		/// Uses given to create an object if file loading is successful. Use corresponding deallocator to remove the texture.
		/// Will return NULL on error.
		//static Texture2D* LoadFromFile(const std::string& sFilename, bool sRGB = false, bool generateMipMaps = true);

		//void SetData(std::uint32_t mipLevel, const ezColor* pData);
		//void SetData(std::uint32_t mipLevel, const unsigned char* pData);

		void GenMipMaps();

		GLenum GetOpenGLTextureType() override { return GetNumMSAASamples() > 0 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D; }
	};

}


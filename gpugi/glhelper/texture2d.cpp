#include "texture2d.hpp"

namespace gl
{
	Texture2D::Texture2D(std::uint32_t width, std::uint32_t height, TextureFormat format, std::int32_t numMipLevels, std::uint32_t numMSAASamples) :
		Texture(width, height, 1, format, numMipLevels, numMSAASamples)
	{
		GL_CALL(glCreateTextures, GL_TEXTURE_2D, 1, &m_textureHandle);
		if (m_numMSAASamples == 0)
			GL_CALL(glTextureStorage2D, m_textureHandle, m_numMipLevels, gl::TextureFormatToGLSizedInternal[static_cast<unsigned int>(format)], m_width, m_height);
		else
			GL_CALL(glTextureStorage2DMultisample, m_textureHandle, m_numMSAASamples, gl::TextureFormatToGLSizedInternal[static_cast<unsigned int>(format)], m_width, m_height, GL_FALSE);
	}

	/*Texture2D* Texture2D::LoadFromFile(const ezString& sFilename, bool sRGB, bool generateMipMaps)
	{
		int uiTexSizeX = -1;
		int uiTexSizeY = -1;
		ezString sAbsolutePath;
		if (ezFileSystem::ResolvePath(sFilename.GetData(), false, &sAbsolutePath, NULL) == EZ_FAILURE)
		{
			ezLog::Error("Couldn't find texture \"%s\".", sFilename.GetData());
			return NULL;
		}
		int numComps = -1;
		stbi_uc* pTextureData = stbi_load(sAbsolutePath.GetData(), &uiTexSizeX, &uiTexSizeY, &numComps, 4);
		if (!pTextureData)
		{
			ezLog::Error("Error loading texture \"%s\".", sAbsolutePath.GetData());
			return NULL;
		}

		Texture2D* poOt = EZ_NEW(pAllocator, Texture2D)(static_cast<std::uint32_t>(uiTexSizeX), static_cast<std::uint32_t>(uiTexSizeY), sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8, generateMipMaps ? -1 : 1);
		poOt->SetData(0, reinterpret_cast<const ezColor8UNorm*>(pTextureData));

		if (generateMipMaps && poOt->GetNumMipLevels() > 1)
			glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(pTextureData);

		return poOt;
	}*/

/*	void Texture2D::SetData(std::uint32_t uiMipLevel, const ezColor* pData)
	{
		Assert(uiMipLevel < m_numMipLevels, "MipLevel %i does not exist, texture has only %i MipMapLevels", uiMipLevel, m_numMipLevels);

		glTextureSubImage2D(m_TextureHandle, uiMipLevel, 0, 0, m_width, m_height,
			GL_RGBA, GL_FLOAT, pData);
	}*/

	void Texture2D::GenMipMaps()
	{
		GL_CALL(glGenerateTextureMipmap, m_textureHandle);
	}

	/*void Texture2D::SetData(std::uint32_t uiMipLevel, const ezColor8UNorm* pData)
	{
		EZ_ASSERT(uiMipLevel < m_numMipLevels, "MipLevel %i does not exist, texture has only %i MipMapLevels", uiMipLevel, m_numMipLevels);

		glTextureSubImage2D(m_TextureHandle,
			uiMipLevel,
			0, 0,
			m_width, m_height,
			GL_RGBA, GL_UNSIGNED_BYTE, pData);
	}*/
}
#include "texture.hpp"
#include "../utilities/assert.hpp"

namespace gl
{

	Texture* Texture::s_pBoundTextures[32];

	Texture::Texture(std::uint32_t width, std::uint32_t height, std::uint32_t depth, TextureFormat format, std::int32_t numMipLevels, std::uint32_t numMSAASamples) :
		m_width(width),
		m_height(height),
		m_depth(depth),

		m_format(format),
		m_numMipLevels(ConvertMipMapSettingToActualCount(numMipLevels, width, height, depth)),

		m_numMSAASamples(numMSAASamples)
	{
		Assert(m_numMipLevels == 1 || numMSAASamples == 0, "Texture must have either zero MSAA samples or only one miplevel!");
		GL_CALL(glGenTextures, 1, &m_TextureHandle);
	}

	Texture::~Texture()
	{
		GL_CALL(glDeleteTextures, 1, &m_TextureHandle);
	}

	std::uint32_t Texture::ConvertMipMapSettingToActualCount(std::int32_t iMipMapSetting, std::uint32_t width, std::uint32_t height, std::uint32_t depth)
	{
		if (iMipMapSetting <= 0)
		{
			std::uint32_t uiNumMipLevels = 0;
			while (width > 0 || height > 0 || depth > 0)
			{
				width /= 2;
				height /= 2;
				depth /= 2;
				++uiNumMipLevels;
			}
			return uiNumMipLevels;
		}

		else
			return iMipMapSetting;
	}

	void Texture::BindImage(GLuint slotIndex, Texture::ImageAccess access, TextureFormat format)
	{
		GL_CALL(glBindImageTexture, slotIndex, m_TextureHandle, 0, GL_TRUE, 0, static_cast<GLenum>(access), gl::TextureFormatToGLSizedInternal[static_cast<unsigned int>(format)]);
	}

	void Texture::ResetImageBinding(GLuint _slotIndex)
	{
		GL_CALL(glBindImageTexture, _slotIndex, 0, 0, false, 0, GL_READ_WRITE, GL_RGBA8);
	}

	void Texture::ClearToZero(std::uint32_t _mipLevel)
	{
		Assert(m_numMipLevels > _mipLevel, "Miplevel " + std::to_string(_mipLevel) + " not available, texture has only " + std::to_string(m_numMipLevels) + " levels!");
		
		unsigned int zeroData[] = { 0, 0, 0, 0 }; // This the maximum known size per pixel.
		GL_CALL(glClearTexImage, m_TextureHandle, _mipLevel, gl::TextureFormatToGLBaseInternal[static_cast<unsigned int>(m_format)], GL_UNSIGNED_INT, zeroData);
	}

	void Texture::Bind(GLuint slotIndex)
	{
		Assert(slotIndex < sizeof(s_pBoundTextures) / sizeof(Texture*), "Can't bind texture to slot " + std::to_string(slotIndex) +". Maximum number of slots is " + std::to_string(sizeof(s_pBoundTextures) / sizeof(Texture*)));
		if (s_pBoundTextures[slotIndex] != this)
		{
			GL_CALL(glActiveTexture, GL_TEXTURE0 + slotIndex);
			GL_CALL(glBindTexture, GetOpenGLTextureType(), m_TextureHandle);
			s_pBoundTextures[slotIndex] = this;
		}
	}
}
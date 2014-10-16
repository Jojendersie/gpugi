#pragma once

#include "gl.hpp"
#include "textureformats.hpp"
#include <cinttypes>

namespace gl
{
	class Texture
	{
	public:
		Texture(std::uint32_t width, std::uint32_t height, std::uint32_t depth, TextureFormat format, std::int32_t numMipLevels, std::uint32_t numMSAASamples = 0);
		~Texture();

		/// Binds texture to the given slot.
		void Bind(GLuint slot);

		/// Different possibilities to access an image.
		enum class ImageAccess
		{
			WRITE = GL_WRITE_ONLY,
			READ = GL_READ_ONLY,
			READ_WRITE = GL_READ_WRITE
		};

		/// Binds as image.
		///
		/// Without redundancy checking! Does not perform checks if texture format is valid (see http://docs.gl/gl4/glBindImageTexture)!
		void BindImage(GLuint slotIndex, ImageAccess access) { BindImage(0, access, m_format); }
		void BindImage(GLuint slotIndex, ImageAccess access, TextureFormat format);

		/// Unbinds image slot.
		static void ResetImageBinding(GLuint _slotIndex);

		/// Clears texture to zero using glClearTexImage (http://docs.gl/gl4/glClearTexImage)
		void ClearToZero(std::uint32_t _mipLevel = 0);

		/// Returns intern OpenGL texture handle.
		TextureId GetInternHandle() { return m_TextureHandle; }

		std::uint32_t GetWidth() const           { return m_width; }
		std::uint32_t GetHeight() const          { return m_height; }
		std::uint32_t GetDepth() const           { return m_depth; }
		std::uint32_t GetNumMipLevels() const    { return m_numMipLevels; }
		std::uint32_t GetNumMSAASamples() const  { return m_numMSAASamples; }
		TextureFormat GetFormat() const          { return m_format; }

		virtual GLenum GetOpenGLTextureType() = 0;

	protected:
		/// Currently bound textures - number is arbitrary!
		/// Not used for image binding
		static Texture* s_pBoundTextures[32];

		TextureId m_TextureHandle;

		const std::uint32_t m_width;
		const std::uint32_t m_height;
		const std::uint32_t m_depth;

		const TextureFormat m_format;
		const std::uint32_t  m_numMipLevels;
		const std::uint32_t  m_numMSAASamples;

	private:
		static std::uint32_t ConvertMipMapSettingToActualCount(std::int32_t iMipMapSetting, std::uint32_t width, std::uint32_t height, std::uint32_t depth = 0);
	};

}

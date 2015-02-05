#pragma once

#include <memory>
#include <string>

namespace gl
{
	class Texture2D;
	class FramebufferObject;
	class ShaderObject;
	class ScreenAlignedTriangle;
}

/// Module for measuring the mean squared error of (float-)texture against a given reference.
///
/// The image to check will be sampled without filter (nearest) - keep that in mind if the size of the reference texture does not match the original.
class TextureMSE
{
public:
	TextureMSE();
	~TextureMSE();

	bool HasValidReferenceImage() const { return m_reference != nullptr; }

	/// Loads a reference image from a PFM file.
	void LoadReferenceFromPFM(const std::string& _filename);

	/// Computes current MSE value.
	float ComputeCurrentMSE(const gl::Texture2D& _texture, unsigned int _divisor);

private:
	std::unique_ptr<gl::ShaderObject> m_shader;
	
	std::unique_ptr<gl::Texture2D> m_reference;
	std::unique_ptr<gl::FramebufferObject> m_errorFrameBuffer;
	std::unique_ptr<gl::Texture2D> m_errorTexture;



	std::unique_ptr<gl::ScreenAlignedTriangle> m_screnTri;
};


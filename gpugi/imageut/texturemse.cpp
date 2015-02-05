#include "texturemse.hpp"
#include "hdrimage.hpp"

#include <glhelper/shaderobject.hpp>
#include <glhelper/texture2d.hpp>
#include <glhelper/framebufferobject.hpp>
#include <glhelper/screenalignedtriangle.hpp>

#include <ei/vector.hpp>

TextureMSE::TextureMSE()
{
	m_shader = std::make_unique<gl::ShaderObject>("TextureMSE");
	m_shader->AddShaderFromFile(gl::ShaderObject::ShaderType::VERTEX, "shader/utils/screenTri.vert");
	m_shader->AddShaderFromFile(gl::ShaderObject::ShaderType::FRAGMENT, "shader/utils/sqerror.frag");
	m_shader->CreateProgram();

	m_screnTri = std::make_unique<gl::ScreenAlignedTriangle>();
}

TextureMSE::~TextureMSE()
{

}

void TextureMSE::LoadReferenceFromPFM(const std::string& _filename)
{
	ei::IVec2 size;
	auto data = LoadPfm(_filename, size);
	if (data)
	{
		m_reference = std::make_unique<gl::Texture2D>(size.x, size.y, gl::TextureFormat::RGB32F, data.get(), gl::TextureSetDataFormat::RGB, gl::TextureSetDataType::FLOAT);
		
		m_errorTexture = std::make_unique<gl::Texture2D>(size.x, size.y, gl::TextureFormat::R32F, 0);
		m_errorFrameBuffer = std::make_unique<gl::FramebufferObject>(gl::FramebufferObject::Attachment(m_errorTexture.get()));
	}
}

float TextureMSE::ComputeCurrentMSE(const gl::Texture2D& _texture, unsigned int _divisor)
{
	float mse = std::numeric_limits<float>::infinity();

	if (m_reference)
	{
		GL_CALL(glMemoryBarrier, GL_TEXTURE_FETCH_BARRIER_BIT);
		GL_CALL(glTextureBarrier);

		_texture.Bind(10);
		m_reference->Bind(13);	// Uses some high texture slot value to avoid overwriting of permanently used texture buffers.
		m_shader->Activate();

		glUniform1ui(0, _divisor);
		
		m_errorFrameBuffer->Bind(true);
		m_screnTri->Draw();
		m_errorFrameBuffer->BindBackBuffer();

		m_errorTexture->GenMipMaps();
		GL_CALL(glTextureBarrier);
		GL_CALL(glMemoryBarrier, GL_TEXTURE_UPDATE_BARRIER_BIT);

		m_errorTexture->ReadImage(m_errorTexture->GetNumMipLevels() - 1, gl::TextureReadFormat::RED, gl::TextureReadType::FLOAT, sizeof(float), &mse);
	}

	return mse;
}

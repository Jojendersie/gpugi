#include "pathtracer.hpp"
#include <glhelper/texture2d.hpp>

#include <fstream>

const ei::UVec2 Pathtracer::m_localSizePathtracer = ei::UVec2(8, 8);

Pathtracer::Pathtracer() :
	m_pathtracerShader("pathtracer")
{
	m_pathtracerShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/pathtracer.comp");
	m_pathtracerShader.CreateProgram();

	// Save shader binary.
/*	{
		GLenum binaryFormat;
		auto binary = m_pathtracerShader.GetProgramBinary(binaryFormat);
		char* data = binary.data();
		std::ofstream shaderBinaryFile("pathtracer_binary.txt");
		shaderBinaryFile.write(data, binary.size());
		shaderBinaryFile.close();
	} */

	InitStandardUBOs(m_pathtracerShader);
	SetNumInitialLightSamples(16);
}

void Pathtracer::SetScreenSize(const ei::IVec2& _newSize)
{
	Renderer::SetScreenSize(_newSize);
	m_backbuffer->BindImage(0, gl::Texture::ImageAccess::READ_WRITE);
}

void Pathtracer::Draw()
{
	++m_iterationCount;
	
	m_pathtracerShader.Activate();
	GL_CALL(glDispatchCompute, m_backbuffer->GetWidth() / m_localSizePathtracer.x, m_backbuffer->GetHeight() / m_localSizePathtracer.y, 1);

	PerIterationBufferUpdate();

	// Ensure that all future fetches will use the modified data.
	// See https://www.opengl.org/wiki/Memory_Model#Ensuring_visibility
	GL_CALL(glMemoryBarrier, GL_TEXTURE_FETCH_BARRIER_BIT);
}
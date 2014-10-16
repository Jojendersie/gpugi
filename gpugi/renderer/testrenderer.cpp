#include "testrenderer.hpp"
#include "../Time/Time.h"

TestRenderer::TestRenderer() :
	testshader("testrenderer")
{
	backbuffer.ClearToZero(0);

	testshader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/testcompute.comp");
	testshader.CreateProgram();
	testubo.Init(testshader, "TestUBO");
}

void TestRenderer::Draw()
{
	testubo["Time"].Set(static_cast<float>(ezTime::Now().GetSeconds()));
	testubo.UpdateGPUData();
	testubo.BindBuffer(0);

	backbuffer.BindImage(0, gl::Texture::ImageAccess::WRITE);

	testshader.Activate();

	GL_CALL(glDispatchCompute, backbuffer.GetWidth() / 32, backbuffer.GetHeight() / 32, 1);

	// Ensure that all future fetches will use the modified data.
	// See https://www.opengl.org/wiki/Memory_Model#Ensuring_visibility
	GL_CALL(glMemoryBarrier, GL_TEXTURE_FETCH_BARRIER_BIT);
}
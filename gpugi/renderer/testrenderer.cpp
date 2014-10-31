#include "testrenderer.hpp"
#include "../Time/Time.h"
#include "../glhelper/texture2d.hpp"
#include "../camera/camera.hpp"
#include <ei/matrix.hpp>

TestRenderer::TestRenderer(const Camera& _initialCamera) :
	m_testshader("testrenderer")
{
	m_backbuffer->ClearToZero(0);

	m_testshader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/testcompute.comp");
	m_testshader.CreateProgram();
	m_testubo.Init(m_testshader, "TestUBO");

	SetCamera(_initialCamera);
}

void TestRenderer::SetCamera(const Camera& _camera)
{
	ei::Vec3 camU, camV, camW;
	_camera.ComputeCameraParams(camU, camV, camW);

	m_testubo.GetBuffer()->Map();
	m_testubo["CameraU"].Set(camU);
	m_testubo["CameraV"].Set(camV);
	m_testubo["CameraW"].Set(camW);
	m_testubo["CameraPosition"].Set(_camera.GetPosition());
}

void TestRenderer::Draw()
{
	m_testubo.GetBuffer()->Map();
	m_testubo["Time"].Set(static_cast<float>(ezTime::Now().GetSeconds()));
	m_testubo.GetBuffer()->Unmap();
	m_testubo.BindBuffer(0);

	m_backbuffer->BindImage(0, gl::Texture::ImageAccess::WRITE);

	m_testshader.Activate();

	GL_CALL(glDispatchCompute, m_backbuffer->GetWidth() / 32, m_backbuffer->GetHeight() / 32, 1);

	// Ensure that all future fetches will use the modified data.
	// See https://www.opengl.org/wiki/Memory_Model#Ensuring_visibility
	GL_CALL(glMemoryBarrier, GL_TEXTURE_FETCH_BARRIER_BIT);
}
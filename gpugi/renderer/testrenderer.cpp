#include "testrenderer.hpp"
#include "../Time/Time.h"
#include "../glhelper/texture2d.hpp"
#include "../camera/camera.hpp"
#include <ei/matrix.hpp>

void CameraMatrixFromLookAt(ei::Vec3 eye, ei::Vec3 lookat, ei::Vec3 up, float hfov, float aspect_ratio,
							ei::Vec3& outU, ei::Vec3& outV, ei::Vec3& outW)
{

}

TestRenderer::TestRenderer(const Camera& initialCamera) :
	testshader("testrenderer")
{
	backbuffer->ClearToZero(0);

	testshader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/testcompute.comp");
	testshader.CreateProgram();
	testubo.Init(testshader, "TestUBO");

	SetCamera(initialCamera);
}

void TestRenderer::SetCamera(const Camera& camera)
{
	testubo["CameraU"].Set(camera.GetCameraU());
	testubo["CameraV"].Set(camera.GetCameraV());
	testubo["CameraW"].Set(camera.GetCameraW());
	testubo["CameraPosition"].Set(camera.GetPosition());
}

void TestRenderer::Draw()
{
	testubo["Time"].Set(static_cast<float>(ezTime::Now().GetSeconds()));
	testubo.UpdateGPUData();
	testubo.BindBuffer(0);

	backbuffer->BindImage(0, gl::Texture::ImageAccess::WRITE);

	testshader.Activate();

	GL_CALL(glDispatchCompute, backbuffer->GetWidth() / 32, backbuffer->GetHeight() / 32, 1);

	// Ensure that all future fetches will use the modified data.
	// See https://www.opengl.org/wiki/Memory_Model#Ensuring_visibility
	GL_CALL(glMemoryBarrier, GL_TEXTURE_FETCH_BARRIER_BIT);
}
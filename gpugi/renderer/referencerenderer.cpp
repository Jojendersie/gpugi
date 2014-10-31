#include "referencerenderer.hpp"

#include "../utilities/random.hpp"
#include "../utilities/flagoperators.hpp"

#include "../glhelper/texture2d.hpp"
#include "../glhelper/screenalignedtriangle.hpp"
#include "../Time/Time.h"

#include "../camera/camera.hpp"

#include <ei/matrix.hpp>


ReferenceRenderer::ReferenceRenderer(const Camera& _initialCamera) :
	m_pathtracerShader("pathtracer"),
	m_backbufferFBO(gl::FramebufferObject::Attachment(m_backbuffer.get()))
{
	m_pathtracerShader.AddShaderFromFile(gl::ShaderObject::ShaderType::VERTEX, "shader/screenTri.vert");
	m_pathtracerShader.AddShaderFromFile(gl::ShaderObject::ShaderType::FRAGMENT, "shader/pathtracer.frag");
	m_pathtracerShader.CreateProgram();

	m_globalConstUBO.Init(m_pathtracerShader, "GlobalConst");
	m_globalConstUBO.GetBuffer()->Map();
	m_globalConstUBO["BackbufferSize"].Set(ei::UVec2(m_backbuffer->GetWidth(), m_backbuffer->GetHeight()));
	m_globalConstUBO.BindBuffer(0);

	m_cameraUBO.Init(m_pathtracerShader, "Camera");
	m_cameraUBO.BindBuffer(1);
	SetCamera(_initialCamera);

	m_perIterationUBO.Init(m_pathtracerShader, "PerIteration", gl::Buffer::Usage::MAP_PERSISTENT | gl::Buffer::Usage::MAP_WRITE | gl::Buffer::Usage::EXPLICIT_FLUSH);
	m_perIterationUBO["FrameSeed"].Set(WangHash(0));
	m_perIterationUBO.BindBuffer(2);

	
	// Additive blending.
	glBlendFunc(GL_ONE, GL_ONE);
}

void ReferenceRenderer::SetCamera(const Camera& _camera)
{
	ei::Vec3 camU, camV, camW;
	_camera.ComputeCameraParams(camU, camV, camW);

	m_cameraUBO.GetBuffer()->Map();
	m_cameraUBO["CameraU"].Set(camU);
	m_cameraUBO["CameraV"].Set(camV);
	m_cameraUBO["CameraW"].Set(camW);
	m_cameraUBO["CameraPosition"].Set(_camera.GetPosition());
	m_cameraUBO.GetBuffer()->Unmap();

	m_iterationCount = 0;
	m_backbuffer->ClearToZero(0);
}

void ReferenceRenderer::OnResize(const ei::UVec2& _newSize)
{
	Renderer::OnResize(_newSize);
	
	m_globalConstUBO.GetBuffer()->Map();
	m_globalConstUBO["BackbufferSize"].Set(_newSize);
	m_globalConstUBO.GetBuffer()->Unmap();
	
	m_iterationCount = 0;
	m_backbuffer->ClearToZero(0);
}

void ReferenceRenderer::Draw()
{
	m_backbufferFBO.Bind(false);
	glEnable(GL_BLEND);

	m_pathtracerShader.Activate();
	m_screenTri.Draw();

	m_backbufferFBO.BindBackBuffer();
	glDisable(GL_BLEND);

	++m_iterationCount;
	m_perIterationUBO["FrameSeed"].Set(WangHash(static_cast<std::uint32_t>(m_iterationCount)));
	GL_CALL(glFlushMappedNamedBufferRange, m_perIterationUBO.GetBuffer()->GetBufferId(), static_cast<GLintptr>(0), m_perIterationUBO.GetBuffer()->GetSize());
}
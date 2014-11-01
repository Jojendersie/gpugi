#include "referencerenderer.hpp"

#include "../utilities/random.hpp"
#include "../utilities/flagoperators.hpp"

#include "../glhelper/texture2d.hpp"
#include "../glhelper/screenalignedtriangle.hpp"
#include "../glhelper/structuredbuffer.hpp"

#include "../Time/Time.h"

#include "../camera/camera.hpp"
#include "../scene/scene.hpp"

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


void ReferenceRenderer::SetScene(std::shared_ptr<Scene> _scene)
{
	m_scene = _scene;

	// Check shader compability.
	auto shaderNodeSize = m_pathtracerShader.GetShaderStorageBufferInfo().at("HierarchyBuffer").iBufferDataSizeByte; //Variables.at("Nodes").iArrayStride;
	Assert(shaderNodeSize == sizeof(Scene::TreeNode<ei::Box>), "Shader tree node is " << shaderNodeSize << "bytes big but expected were " << sizeof(Scene::TreeNode<ei::Box>) << "bytes");
	auto shaderVertexSize = m_pathtracerShader.GetShaderStorageBufferInfo().at("VertexBuffer").iBufferDataSizeByte; //.Variables.at("Vertices").iArrayStride;
	Assert(shaderVertexSize == sizeof(FileDecl::Vertex), "Shader vertex is " << shaderVertexSize << "bytes big but expected were " << sizeof(FileDecl::Vertex) << "bytes");
	auto shaderLeafSize = m_pathtracerShader.GetShaderStorageBufferInfo().at("LeafBuffer").iBufferDataSizeByte; //.Variables.at("Leafs").iArrayStride;
	auto expectedLeafSize = sizeof(Scene::Triangle) * m_scene->GetNumTrianglesPerLeaf();
	Assert(shaderLeafSize == expectedLeafSize, "Shader leaf is " << shaderLeafSize << "bytes big but expected were " << expectedLeafSize << "bytes");


	// Bind buffer
	m_hierarchyBuffer.reset(new gl::StructuredBufferView());
	m_hierarchyBuffer->Init(m_scene->GetHierarchyBuffer(), "HierarchyBuffer");
	m_pathtracerShader.BindSSBO(*m_hierarchyBuffer);

	m_vertexBuffer.reset(new gl::StructuredBufferView());
	m_vertexBuffer->Init(m_scene->GetVertexBuffer(), "VertexBuffer");
	m_pathtracerShader.BindSSBO(*m_vertexBuffer);

	m_leafBuffer.reset(new gl::StructuredBufferView());
	m_leafBuffer->Init(m_scene->GetTriangleBuffer(), "LeafBuffer");
	m_pathtracerShader.BindSSBO(*m_leafBuffer);
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
#include "referencerenderer.hpp"

#include "../utilities/random.hpp"
#include "../utilities/flagoperators.hpp"

#include "../glhelper/texture2d.hpp"
#include "../glhelper/screenalignedtriangle.hpp"
#include "../glhelper/texturebuffer.hpp"

#include "../Time/Time.h"

#include "../camera/camera.hpp"
#include "../scene/scene.hpp"
#include "../scene/lighttrianglesampler.hpp"

#include <ei/matrix.hpp>

#include <fstream>
//#include "../utilities/color.hpp"


ReferenceRenderer::ReferenceRenderer(const Camera& _initialCamera) :
	m_pathtracerShader("pathtracer"),
	m_blendShader("blend"),
	m_backbufferFBO(gl::FramebufferObject::Attachment(m_backbuffer.get())),

	m_numInitialLightSamples(16)
{
	m_pathtracerShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/pathtracer.comp");
	m_pathtracerShader.CreateProgram();

	// Save shader binary.
	{
		GLenum binaryFormat;
		auto binary = m_pathtracerShader.GetProgramBinary(binaryFormat);
		char* data = binary.data();
		std::ofstream shaderBinaryFile("pathtracer_binary.txt");
		shaderBinaryFile.write(data, binary.size());
		shaderBinaryFile.close();
	}

	// Uniform buffers
	{
		m_blendShader.AddShaderFromFile(gl::ShaderObject::ShaderType::VERTEX, "shader/screenTri.vert");
		m_blendShader.AddShaderFromFile(gl::ShaderObject::ShaderType::FRAGMENT, "shader/displayHDR.frag");
		m_blendShader.CreateProgram();

		m_globalConstUBO.Init(m_pathtracerShader, "GlobalConst");
		m_globalConstUBO.GetBuffer()->Map();
		m_globalConstUBO["BackbufferSize"].Set(ei::UVec2(m_backbuffer->GetWidth(), m_backbuffer->GetHeight()));
		m_globalConstUBO.BindBuffer(0);

		m_cameraUBO.Init(m_pathtracerShader, "Camera");
		m_cameraUBO.BindBuffer(1);
		SetCamera(_initialCamera);

		m_perIterationUBO.Init(m_pathtracerShader, "PerIteration", gl::Buffer::Usage::MAP_PERSISTENT | gl::Buffer::Usage::MAP_WRITE | gl::Buffer::Usage::EXPLICIT_FLUSH);
		m_perIterationUBO.BindBuffer(2);

		m_materialUBO.Init(m_pathtracerShader, "UMaterials");
		m_materialUBO.BindBuffer(3);
	}

	//m_iterationBuffer.reset(new gl::Texture2D(m_backbuffer->GetWidth(), m_backbuffer->GetHeight(), gl::TextureFormat::RGBA32F, 1, 0));

	m_initialLightSampleBuffer.reset(new gl::TextureBufferView());
	m_initialLightSampleBuffer->Init(
		std::make_shared<gl::Buffer>(static_cast<std::uint32_t>(sizeof(LightTriangleSampler::LightSample) * m_numInitialLightSamples),
					gl::Buffer::Usage::MAP_WRITE | gl::Buffer::Usage::MAP_PERSISTENT | gl::Buffer::Usage::EXPLICIT_FLUSH), gl::TextureBufferFormat::RGBA32F);
	m_initialLightSampleBuffer->BindBuffer(4);


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
	m_lightTriangleSampler.SetScene(m_scene);

	// Check shader compability.
	//auto shaderNodeSize = m_pathtracerShader.GetShaderStorageBufferInfo().at("HierarchyBuffer").iBufferDataSizeByte; //Variables.at("Nodes").iArrayStride;
	//Assert(shaderNodeSize == sizeof(Scene::TreeNode<ei::Box>), "Shader tree node is " << shaderNodeSize << "bytes big but expected were " << sizeof(Scene::TreeNode<ei::Box>) << "bytes");
	//auto shaderVertexSize = m_pathtracerShader.GetShaderStorageBufferInfo().at("VertexBuffer").iBufferDataSizeByte; //.Variables.at("Vertices").iArrayStride;
	//Assert(shaderVertexSize == sizeof(FileDecl::Vertex), "Shader vertex is " << shaderVertexSize << "bytes big but expected were " << sizeof(FileDecl::Vertex) << "bytes");
	//auto shaderLeafSize = m_pathtracerShader.GetShaderStorageBufferInfo().at("LeafBuffer").iBufferDataSizeByte; //.Variables.at("Leafs").iArrayStride;
	//auto expectedLeafSize = sizeof(Scene::Triangle) * m_scene->GetNumTrianglesPerLeaf();
	//Assert(shaderLeafSize == expectedLeafSize, "Shader leaf is " << shaderLeafSize << "bytes big but expected were " << expectedLeafSize << "bytes");


	// Bind buffer
	m_triangleBuffer.reset(new gl::TextureBufferView());
	m_triangleBuffer->Init(m_scene->GetTriangleBuffer(), gl::TextureBufferFormat::RGBA32I);
	m_triangleBuffer->BindBuffer(0);

	m_vertexPositionBuffer.reset(new gl::TextureBufferView());
	m_vertexPositionBuffer->Init(m_scene->GetVertexPositionBuffer(), gl::TextureBufferFormat::RGB32F);
	m_vertexPositionBuffer->BindBuffer(1);

	m_vertexInfoBuffer.reset(new gl::TextureBufferView());
	m_vertexInfoBuffer->Init(m_scene->GetVertexInfoBuffer(), gl::TextureBufferFormat::RGBA32F);
	m_vertexInfoBuffer->BindBuffer(2);

	m_hierarchyBuffer.reset(new gl::TextureBufferView());
	m_hierarchyBuffer->Init(m_scene->GetHierarchyBuffer(), gl::TextureBufferFormat::RGBA32F);
	m_hierarchyBuffer->BindBuffer(3);

	// Upload materials / set textures
	m_materialUBO.GetBuffer()->Map();
	m_materialUBO.Set(&m_scene->GetMaterials().front(), 0, uint32_t(m_scene->GetMaterials().size() * sizeof(Scene::Material)));

	// Needs an update, to start with correct light samples.
	PerIterationBufferUpdate();
}

void ReferenceRenderer::OnResize(const ei::UVec2& _newSize)
{
	Renderer::OnResize(_newSize);

	//m_iterationBuffer.reset(new gl::Texture2D(_newSize.x, _newSize.y, gl::TextureFormat::RGBA32F, 1, 0));
	
	m_globalConstUBO.GetBuffer()->Map();
	m_globalConstUBO["BackbufferSize"].Set(_newSize);
	m_globalConstUBO.GetBuffer()->Unmap();
	
	m_iterationCount = 0;
	m_backbuffer->ClearToZero(0);
}

void ReferenceRenderer::PerIterationBufferUpdate()
{
	m_perIterationUBO.GetBuffer()->Map();
	m_perIterationUBO["FrameSeed"].Set(WangHash(static_cast<std::uint32_t>(m_iterationCount)));
	m_perIterationUBO["NumLightSamples"].Set(m_numInitialLightSamples); // todo
	m_perIterationUBO.GetBuffer()->Flush();

	// There could be some performance gain in double/triple buffering this buffer.
	m_lightTriangleSampler.GenerateRandomSamples(static_cast<LightTriangleSampler::LightSample*>(m_initialLightSampleBuffer->GetBuffer()->Map()), m_numInitialLightSamples);
	m_initialLightSampleBuffer->GetBuffer()->Flush();
}

void ReferenceRenderer::Draw()
{
	++m_iterationCount;
	{
		//m_iterationBuffer->BindImage(0, gl::Texture::ImageAccess::WRITE);
		m_backbuffer->BindImage(0, gl::Texture::ImageAccess::READ_WRITE);

		m_pathtracerShader.Activate();
		GL_CALL(glDispatchCompute, m_backbuffer->GetWidth() / 16, m_backbuffer->GetHeight() / 16, 1);

		// Ensure that all future fetches will use the modified data.
		// See https://www.opengl.org/wiki/Memory_Model#Ensuring_visibility
		GL_CALL(glMemoryBarrier, GL_TEXTURE_FETCH_BARRIER_BIT);
	}

	PerIterationBufferUpdate();

	/*{
		m_backbufferFBO.Bind(false);
		glEnable(GL_BLEND);

		m_iterationBuffer->Bind(0);
		m_blendShader.Activate();

		m_screenTri.Draw();

		glDisable(GL_BLEND);
		m_backbufferFBO.BindBackBuffer();
	}*/
}
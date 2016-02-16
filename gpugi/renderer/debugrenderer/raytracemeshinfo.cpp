#include "raytracemeshinfo.hpp"
#include <glhelper/screenalignedtriangle.hpp>
#include <glhelper/buffer.hpp>

#include "control/scriptprocessing.hpp"
#include "control/globalconfig.hpp"
#include "scene/scene.hpp"

const std::string RaytraceMeshInfo::Name = "Raytrace Meshinfo";

RaytraceMeshInfo::RaytraceMeshInfo(Renderer& _parentRenderer) :
	DebugRenderer(_parentRenderer),
	m_screenTri(new gl::ScreenAlignedTriangle()),
	m_infoShader("RaytraceMeshInfo")
{
	m_infoShader.AddShaderFromFile(gl::ShaderObject::ShaderType::VERTEX, "shader/utils/screenTri.vert", "#define AABOX_BVH\n");
	m_infoShader.AddShaderFromFile(gl::ShaderObject::ShaderType::FRAGMENT, "shader/debug/raytracemeshinfo.frag", "#define AABOX_BVH\n");
	m_infoShader.CreateProgram();

	auto metaData = m_infoShader.GetUniformBufferInfo()["DebugSettings"];
	m_settingsUBO = std::make_unique<gl::Buffer>(metaData.bufferDataSizeByte, gl::Buffer::UsageFlag::MAP_WRITE);
	m_settingsUBO->BindUniformBuffer(8);

	GlobalConfig::AddParameter("meshinfotype", { 0 }, std::string("Displayed meshinfo type:\n") +
																"0: Normal (*.5 + .5)\n" +
																"1: Diffuse color\n" + 
																"2: Opacity color\n" + 
																"3: Reflection color\n" +
																"4: Triangle ID");
	GlobalConfig::AddListener("meshinfotype", "RaytraceMeshInfo", [&, metaData](const GlobalConfig::ParameterType& p) {
		gl::MappedUBOView dataView(metaData, m_settingsUBO->Map(gl::Buffer::MapType::WRITE, gl::Buffer::MapWriteFlag::NONE));
		dataView["DebugType"].Set(p[0].As<std::int32_t>());
		m_settingsUBO->Unmap();
	});
	GlobalConfig::SetParameter("meshinfotype", { 0 });
}

RaytraceMeshInfo::~RaytraceMeshInfo()
{
	GlobalConfig::RemoveParameter("meshinfotype");
}

void RaytraceMeshInfo::SetScene(std::shared_ptr<Scene> _scene)
{
	RecompileShaders(_scene->GetBvhTypeDefineString());
}

void RaytraceMeshInfo::Draw()
{
	m_infoShader.Activate();
	m_screenTri->Draw();
}

void RaytraceMeshInfo::RecompileShaders(const std::string& _additionalDefines)
{
	m_infoShader.AddShaderFromFile(gl::ShaderObject::ShaderType::VERTEX, "shader/utils/screenTri.vert", _additionalDefines);
	m_infoShader.AddShaderFromFile(gl::ShaderObject::ShaderType::FRAGMENT, "shader/debug/raytracemeshinfo.frag", _additionalDefines);
	m_infoShader.CreateProgram();
}

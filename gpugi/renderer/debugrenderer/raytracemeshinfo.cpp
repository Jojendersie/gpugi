#include "raytracemeshinfo.hpp"
#include <glhelper/screenalignedtriangle.hpp>
#include <glhelper/uniformbuffer.hpp>

#include "../../control/scriptprocessing.hpp"
#include "../../control/globalconfig.hpp"

const std::string RaytraceMeshInfo::Name = "Raytrace Meshinfo";
const ei::UVec2 RaytraceMeshInfo::m_localSize= ei::UVec2(8, 8);

RaytraceMeshInfo::RaytraceMeshInfo(const Renderer& _parentRenderer) :
	DebugRenderer(_parentRenderer),
	m_screenTri(new gl::ScreenAlignedTriangle()),
	m_infoShader("RaytraceMeshInfo")
{
	m_infoShader.AddShaderFromFile(gl::ShaderObject::ShaderType::VERTEX, "shader/debug/screenTri.vert");
	m_infoShader.AddShaderFromFile(gl::ShaderObject::ShaderType::FRAGMENT, "shader/debug/raytracemeshinfo.frag");
	m_infoShader.CreateProgram();

	m_settingsUBO = std::make_unique<gl::UniformBufferView>();
	m_settingsUBO->Init(m_infoShader, "DebugSettings");
	m_settingsUBO->BindBuffer(8);

	GlobalConfig::AddParameter("meshinfotype", { 0 }, std::string("Displayed meshinfo type:\n") +
																"0: Normal (*.5 + .5)\n" +
																"1: Diffuse color\n" + 
																"2: Opacity color\n" + 
																"3: Reflection color");
	GlobalConfig::AddListener("meshinfotype", "RaytraceMeshInfo", [&](const GlobalConfig::ParameterType& p) {
		m_settingsUBO->GetBuffer()->Map();
		(*m_settingsUBO)["DebugType"].Set(p[0].As<std::int32_t>());
		m_settingsUBO->GetBuffer()->Unmap();
	});
	GlobalConfig::SetParameter("meshinfotype", { 0 });
}

RaytraceMeshInfo::~RaytraceMeshInfo()
{
	GlobalConfig::RemoveParameter("meshinfotype");
}

void RaytraceMeshInfo::Draw()
{
	m_infoShader.Activate();
	m_screenTri->Draw();
}
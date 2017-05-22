#include "photonmapper.hpp"
#include "renderersystem.hpp"
#include "debugrenderer/raytracemeshinfo.hpp"
#include "debugrenderer/hierarchyvisualization.hpp"
#include "scene/scene.hpp"
#include "ei/prime.hpp"
#include <glhelper/texture2d.hpp>
#include <glhelper/texturecubemap.hpp>

#include <fstream>

const ei::UVec2 PhotonMapper::m_localWGSizeDistributionShader = ei::UVec2(64, 1);
const ei::UVec2 PhotonMapper::m_localWGSizeGatherShader = ei::UVec2(8, 8);

PhotonMapper::PhotonMapper(RendererSystem& _rendererSystem) :
	Renderer(_rendererSystem),
	m_photonDistributionShader("photonDistribution"),
	m_gatherShader("gatherPhoton"),
	m_numPhotonsPerLightSample(1 << 11),
	m_queryRadius(0.01f)
{
	// Save shader binary.
/*	{
		GLenum binaryFormat;
		auto binary = m_pathtracerShader.GetProgramBinary(binaryFormat);
		char* data = binary.data();
		std::ofstream shaderBinaryFile("pathtracer_binary.txt");
		shaderBinaryFile.write(data, binary.size());
		shaderBinaryFile.close();
	} */

	m_rendererSystem.SetNumInitialLightSamples(128);
}

void PhotonMapper::SetScene(std::shared_ptr<Scene> _scene)
{
	RecompileShaders(_scene->GetBvhTypeDefineString());
}

void PhotonMapper::SetScreenSize(const gl::Texture2D& _newBackbuffer)
{
	_newBackbuffer.BindImage(0, gl::Texture::ImageAccess::READ_WRITE);
}

void PhotonMapper::SetEnvironmentMap(std::shared_ptr<gl::TextureCubemap> _envMap)
{
	if(_envMap)
		_envMap->Bind(12);
}

void PhotonMapper::Draw()
{
	// Create resources?
	if(!m_photonMap)
	{
		CreateBuffers();
	}

	m_photonMapperUBO->BindUniformBuffer(4);
	ei::UVec4 mask(0xffffffff);
	GL_CALL(glClearNamedBufferData, m_photonMap->GetInternHandle(), GL_RG8UI, GL_RG, GL_UNSIGNED_INT, &mask);
	m_photonMapData->ClearToZero();
	m_photonMap->BindShaderStorageBuffer(6);
	m_photonMapData->BindShaderStorageBuffer(7);
	GL_CALL(glMemoryBarrier, GL_SHADER_STORAGE_BARRIER_BIT);

	m_photonDistributionShader.Activate();
	GL_CALL(glDispatchCompute, m_rendererSystem.GetNumInitialLightSamples() * m_numPhotonsPerLightSample / m_localWGSizeDistributionShader.x, 1, 1);
	GL_CALL(glMemoryBarrier, GL_SHADER_STORAGE_BARRIER_BIT);

	m_gatherShader.Activate();
	GL_CALL(glDispatchCompute, m_rendererSystem.GetBackbuffer().GetWidth() / m_localWGSizeGatherShader.x, m_rendererSystem.GetBackbuffer().GetHeight() / m_localWGSizeGatherShader.y, 1);

	//m_rendererSystem.DispatchShowLightCacheShader();
}

void PhotonMapper::RecompileShaders(const std::string& _additionalDefines)
{
	std::string additionalDefines = _additionalDefines;
#ifdef SHOW_SPECIFIC_PATHLENGTH
	additionalDefines += "#define SHOW_SPECIFIC_PATHLENGTH " + std::to_string(SHOW_SPECIFIC_PATHLENGTH) + "\n";
#endif

	m_photonDistributionShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/photonmapper/photondistribution.comp", additionalDefines);
	m_photonDistributionShader.CreateProgram();
	m_gatherShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/photonmapper/gather.comp", additionalDefines);
	m_gatherShader.CreateProgram();
}

void PhotonMapper::CreateBuffers()
{
	// Determine a prime hash map size with at least 1.5x as much space as there
	// are photons. Although, the real size depends on the number of filled cells this
	// is a conservative heuristic.
	uint minPhotonMapSize = (m_numPhotonsPerLightSample * m_rendererSystem.GetNumInitialLightSamples() * 3 / 2);
	uint photonMapSize = ei::nextPrimeGreaterOrEqual(minPhotonMapSize);
	m_photonMap = std::make_unique<gl::Buffer>(photonMapSize * 2 * 4, gl::Buffer::IMMUTABLE);
	m_photonMapData = std::make_unique<gl::Buffer>(5 * m_numPhotonsPerLightSample * m_rendererSystem.GetNumInitialLightSamples() * 8 * 4 + 4 * 4, gl::Buffer::IMMUTABLE);

	m_photonMapperUBOInfo = m_photonDistributionShader.GetUniformBufferInfo().find("PhotonMapperUBO")->second;
	m_photonMapperUBO = std::make_unique<gl::Buffer>(m_photonMapperUBOInfo.bufferDataSizeByte, gl::Buffer::MAP_WRITE);
	m_photonMapperUBO->BindUniformBuffer(4);

	gl::MappedUBOView mapView(m_photonMapperUBOInfo, m_photonMapperUBO->Map(gl::Buffer::MapType::WRITE, gl::Buffer::MapWriteFlag::NONE));
	mapView["HashMapSize"].Set(photonMapSize);
	mapView["PhotonQueryRadiusSq"].Set(m_queryRadius * m_queryRadius);
	mapView["HashGridSpacing"].Set(m_queryRadius * 2.01f);
	mapView["NumPhotonsPerLightSample"].Set(static_cast<std::int32_t>(m_numPhotonsPerLightSample));
	m_photonMapperUBO->Unmap();
}

#include "pixelmaplighttracer.hpp"
#include "renderersystem.hpp"
#include "debugrenderer/raytracemeshinfo.hpp"
#include "debugrenderer/hierarchyvisualization.hpp"
#include "scene/scene.hpp"
#include "utilities/logger.hpp"
#include <ei/prime.hpp>
#include <glhelper/texture2d.hpp>
#include <glhelper/texturecubemap.hpp>

#include <fstream>

const ei::UVec2 PixelMapLighttracer::m_localWGSizePhotonShader = ei::UVec2(64, 1);
const ei::UVec2 PixelMapLighttracer::m_localWGSizeImportonShader = ei::UVec2(8, 8);

PixelMapLighttracer::PixelMapLighttracer(RendererSystem& _rendererSystem) :
	Renderer(_rendererSystem),
	m_photonTracingShader("photonTracing"),
	m_importonDistributionShader("importonDistribution"),
	m_numPhotonsPerLightSample(1 << 11),
	m_queryRadius(0.005f)
{
	m_rendererSystem.SetNumInitialLightSamples(128);
}

void PixelMapLighttracer::SetScene(std::shared_ptr<Scene> _scene)
{
	RecompileShaders(_scene->GetBvhTypeDefineString());
}

void PixelMapLighttracer::SetScreenSize(const gl::Texture2D& _newBackbuffer)
{
	_newBackbuffer.BindImage(0, gl::Texture::ImageAccess::READ_WRITE);

	m_lockTexture.reset(new gl::Texture2D(_newBackbuffer.GetWidth(), _newBackbuffer.GetHeight(), gl::TextureFormat::R32UI));
	m_lockTexture->ClearToZero(0);
	m_lockTexture->BindImage(1, gl::Texture::ImageAccess::READ_WRITE);
}

void PixelMapLighttracer::SetEnvironmentMap(std::shared_ptr<gl::TextureCubemap> _envMap)
{
	if(_envMap)
		_envMap->Bind(12);
}

void PixelMapLighttracer::Draw()
{
	// Create resources?
	if(!m_pixelMap)
	{
		CreateBuffers();
	}

	m_pixelMapLTUBO->BindUniformBuffer(4);
	ei::UVec4 mask(0xffffffff);
	GL_CALL(glClearNamedBufferData, m_pixelMap->GetInternHandle(), GL_RG8UI, GL_RG, GL_UNSIGNED_INT, &mask);
	m_pixelMapData->ClearToZero();
	m_pixelMap->BindShaderStorageBuffer(6);
	m_pixelMapData->BindShaderStorageBuffer(7);
	GL_CALL(glMemoryBarrier, GL_SHADER_STORAGE_BARRIER_BIT);

	m_importonDistributionShader.Activate();
	GL_CALL(glDispatchCompute, m_rendererSystem.GetBackbuffer().GetWidth() / m_localWGSizeImportonShader.x, m_rendererSystem.GetBackbuffer().GetHeight() / m_localWGSizeImportonShader.y, 1);
	GL_CALL(glMemoryBarrier, GL_SHADER_STORAGE_BARRIER_BIT);

	m_photonTracingShader.Activate();
	GL_CALL(glDispatchCompute, m_rendererSystem.GetNumInitialLightSamples() * m_numPhotonsPerLightSample / m_localWGSizePhotonShader.x, 1, 1);
}

void PixelMapLighttracer::RecompileShaders(const std::string& _additionalDefines)
{
	std::string additionalDefines = _additionalDefines;
#ifdef SHOW_SPECIFIC_PATHLENGTH
	additionalDefines += "#define SHOW_SPECIFIC_PATHLENGTH " + std::to_string(SHOW_SPECIFIC_PATHLENGTH) + "\n";
#endif

	m_photonTracingShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/pixelmaplt/photontracing.comp", additionalDefines);
	m_photonTracingShader.CreateProgram();
	m_importonDistributionShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/pixelmaplt/importondistribution.comp", additionalDefines);
	m_importonDistributionShader.CreateProgram();
}

void PixelMapLighttracer::CreateBuffers()
{
	uint numPixels = m_rendererSystem.GetBackbuffer().GetWidth() * m_rendererSystem.GetBackbuffer().GetHeight();
	// Determine a prime hash map size with at least 1.5x as much space as there
	// are photons. Although, the real size depends on the number of filled cells this
	// is a conservative heuristic.
	uint minPixelMapSize = (numPixels * 3 / 2);
	uint photonMapSize = ei::nextPrimeGreaterOrEqual(minPixelMapSize);
	m_pixelMap = std::make_unique<gl::Buffer>(photonMapSize * 2 * 4, gl::Buffer::IMMUTABLE);
	m_pixelMapData = std::make_unique<gl::Buffer>(numPixels * 8 * 4 + 4 * 4, gl::Buffer::IMMUTABLE);
	LOG_LVL2("Allocated " << (m_pixelMap->GetSize() + m_pixelMapData->GetSize()) / (1024*1024) << " MB for importon map.");

	m_pixelMapLTUBOInfo = m_importonDistributionShader.GetUniformBufferInfo().find("ImportonMapperUBO")->second;
	m_pixelMapLTUBO = std::make_unique<gl::Buffer>(m_pixelMapLTUBOInfo.bufferDataSizeByte, gl::Buffer::MAP_WRITE);
	m_pixelMapLTUBO->BindUniformBuffer(4);

	gl::MappedUBOView mapView(m_pixelMapLTUBOInfo, m_pixelMapLTUBO->Map(gl::Buffer::MapType::WRITE, gl::Buffer::MapWriteFlag::NONE));
	mapView["HashMapSize"].Set(photonMapSize);
	mapView["PhotonQueryRadiusSq"].Set(m_queryRadius * m_queryRadius);
	mapView["HashGridSpacing"].Set(m_queryRadius * 2.01f);
	mapView["NumPhotonsPerLightSample"].Set(static_cast<std::int32_t>(m_numPhotonsPerLightSample));
	m_pixelMapLTUBO->Unmap();
}

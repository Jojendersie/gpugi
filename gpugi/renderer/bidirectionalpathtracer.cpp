#include "bidirectionalpathtracer.hpp"
#include <glhelper/texture2d.hpp>
#include <glhelper/shaderstoragebuffer.hpp>
#include "../utilities/flagoperators.hpp"
#include "../utilities/logger.hpp"

const unsigned int BidirectionalPathtracer::m_localSizeLightPathtracer = 8*8;
const ei::UVec2 BidirectionalPathtracer::m_localSizePathtracer = ei::UVec2(8, 8);

BidirectionalPathtracer::BidirectionalPathtracer() :
	m_lighttraceShader("lighttracer_bidir"),
	m_pathtraceShader("pathtracer_bidir"),
	m_warmupLighttraceShader("lighttracer_warmup_bidir")
{
	std::string additionalDefines;
#ifdef SHOW_ONLY_PATHLENGTH
	additionalDefines += "#define SHOW_ONLY_PATHLENGTH " + std::to_string(SHOW_ONLY_PATHLENGTH) + "\n";
#endif

	m_pathtraceShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/pathtracer_bidir.comp", additionalDefines);
	m_pathtraceShader.CreateProgram();
	additionalDefines += "#define SAVE_LIGHT_CACHE\n";
	m_lighttraceShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/lighttracer.comp", additionalDefines);
	m_lighttraceShader.CreateProgram();
	m_warmupLighttraceShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/lighttracer.comp", additionalDefines + "#define SAVE_LIGHT_CACHE_WARMUP\n");
	m_warmupLighttraceShader.CreateProgram();

	// Assure that LightCacheCount/LightCache SSBOs are defined the same in all shaders
#ifndef NDEBUG
	auto pathtrace_LightCacheCount = m_lighttraceShader.GetShaderStorageBufferInfo().find("LightCacheCount")->second;
	auto lighttrace_LightCacheCount = m_lighttraceShader.GetShaderStorageBufferInfo().find("LightCacheCount")->second;
	auto warmupLighttrace_LightCacheCount = m_warmupLighttraceShader.GetShaderStorageBufferInfo().find("LightCacheCount")->second;

	Assert(pathtrace_LightCacheCount.iBufferBinding == lighttrace_LightCacheCount.iBufferBinding &&
		pathtrace_LightCacheCount.iBufferBinding == warmupLighttrace_LightCacheCount.iBufferBinding &&
		pathtrace_LightCacheCount.iBufferDataSizeByte == lighttrace_LightCacheCount.iBufferDataSizeByte &&
		pathtrace_LightCacheCount.iBufferDataSizeByte == warmupLighttrace_LightCacheCount.iBufferDataSizeByte, "Different definitions of LightCacheCount SSBO within shaders!");

	auto pathtrace_LightCache = m_lighttraceShader.GetShaderStorageBufferInfo().find("LightCache")->second;
	auto lighttrace_LightCache = m_lighttraceShader.GetShaderStorageBufferInfo().find("LightCache")->second;
	auto warmupLighttrace_LightCache = m_warmupLighttraceShader.GetShaderStorageBufferInfo().find("LightCache")->second;

	Assert(pathtrace_LightCache.iBufferBinding == lighttrace_LightCache.iBufferBinding &&
		pathtrace_LightCache.iBufferBinding == warmupLighttrace_LightCache.iBufferBinding &&
		pathtrace_LightCache.iBufferDataSizeByte == lighttrace_LightCache.iBufferDataSizeByte &&
		pathtrace_LightCache.iBufferDataSizeByte == warmupLighttrace_LightCache.iBufferDataSizeByte, "Different definitions of LightCache SSBO within shaders!");
#endif


	m_lightpathtraceUBO.Init(m_lighttraceShader, "LightPathTrace");
	m_lightpathtraceUBO.BindBuffer(4);

	m_lightCacheFillCounter.reset(new gl::ShaderStorageBufferView());
	m_lightCacheFillCounter->Init(std::make_shared<gl::Buffer>(4, gl::Buffer::Usage::MAP_READ), "LightCacheCount");
	m_warmupLighttraceShader.BindSSBO(*m_lightCacheFillCounter);

	InitStandardUBOs(m_lighttraceShader);
	SetNumInitialLightSamples(128);
}

void BidirectionalPathtracer::SetScreenSize(const ei::IVec2& _newSize)
{
	Renderer::SetScreenSize(_newSize);
	m_backbuffer->BindImage(0, gl::Texture::ImageAccess::READ_WRITE);

	m_iterationCount = 0;

	// Lock texture for light-camera path connections.
	m_lockTexture.reset(new gl::Texture2D(_newSize.x, _newSize.y, gl::TextureFormat::R32UI));
	m_lockTexture->ClearToZero(0);
	m_lockTexture->BindImage(1, gl::Texture::ImageAccess::READ_WRITE);

	// Rule: Every block (size = m_localSizeLightPathtracer) should work with the same initial light sample!
	int numPixels = _newSize.x * _newSize.y;
	m_numRaysPerLightSample = std::max(m_localSizeLightPathtracer, (numPixels / GetNumInitialLightSamples() / m_localSizeLightPathtracer) * m_localSizeLightPathtracer);

	// Set constants ...
	m_lightpathtraceUBO.GetBuffer()->Map();
	m_lightpathtraceUBO["NumRaysPerLightSample"].Set(static_cast<std::int32_t>(m_numRaysPerLightSample));
	m_lightpathtraceUBO["LightRayPixelWeight"].Set(ei::PI * 2.0f / m_numRaysPerLightSample);
	m_lightpathtraceUBO["LightCacheCapacity"].Set(std::numeric_limits<std::uint32_t>().max()); // Not known yet.
	m_lightpathtraceUBO.GetBuffer()->Unmap();

	m_needToDetermineNeededLightCacheCapacity = true;
}

void BidirectionalPathtracer::CreateLightCacheWithCapacityEstimate()
{
	// Perform Warmup to find out how large the lightcache should be.
	LOG_LVL2("Starting bpt warmup to estimate needed light cache capacity...");
	m_warmupLighttraceShader.Activate();
	static unsigned int numWarmupRuns = 8;
	std::uint64_t numCacheEntries = 0;
	for (unsigned int run = 0; run < numWarmupRuns; ++run)
	{
		GL_CALL(glDispatchCompute, GetNumInitialLightSamples() * m_numRaysPerLightSample / m_localSizeLightPathtracer, 1, 1);
		GL_CALL(glMemoryBarrier, GL_SHADER_STORAGE_BARRIER_BIT);

		numCacheEntries += *reinterpret_cast<const std::uint32_t*>(m_lightCacheFillCounter->GetBuffer()->Map());
		m_lightCacheFillCounter->GetBuffer()->Unmap();

		// Driver workaround: keep in fast memory while computing
		// A map may move the memory block to slower memory where it stays.
		m_lightCacheFillCounter.reset(new gl::ShaderStorageBufferView());
		m_lightCacheFillCounter->Init(std::make_shared<gl::Buffer>(4, gl::Buffer::Usage::MAP_READ), "LightCacheCount");
		m_warmupLighttraceShader.BindSSBO(*m_lightCacheFillCounter);
	}
	numCacheEntries /= numWarmupRuns;
	m_lightCacheCapacity = static_cast<unsigned int>(numCacheEntries + numCacheEntries / 10); // Add 10% safety margin.
#ifdef SHOW_ONLY_PATHLENGTH
	const unsigned int lightCacheEntrySize = sizeof(float) * 4 * 4 + sizeof(std::uint32_t);
#else
	const unsigned int lightCacheEntrySize = sizeof(float) * 4 * 4;
#endif
	unsigned int lightCacheSizeInBytes = m_lightCacheCapacity * lightCacheEntrySize;

	LOG_LVL2("... bpt warmup done. Reserving " << lightCacheSizeInBytes /1024/1024 << "mb light sample cache!");

	m_lightCacheFillCounter.reset(new gl::ShaderStorageBufferView());
	m_lightCacheFillCounter->Init(std::make_shared<gl::Buffer>(4, gl::Buffer::Usage::IMMUTABLE), "LightCacheCount");
	m_warmupLighttraceShader.BindSSBO(*m_lightCacheFillCounter);

	// Create light cache
	m_lightpathtraceUBO.GetBuffer()->Map();
	m_lightpathtraceUBO["LightCacheCapacity"].Set(m_lightCacheCapacity);
	m_lightpathtraceUBO.GetBuffer()->Unmap();
	m_lightCache.reset(new gl::ShaderStorageBufferView());
	m_lightCache->Init(std::make_shared<gl::Buffer>(lightCacheSizeInBytes, gl::Buffer::Usage::IMMUTABLE), "LightCache");
	m_lighttraceShader.BindSSBO(*m_lightCache);
}

void BidirectionalPathtracer::Draw()
{
	if (m_needToDetermineNeededLightCacheCapacity)
	{
		CreateLightCacheWithCapacityEstimate();
		m_needToDetermineNeededLightCacheCapacity = false;
	}

	++m_iterationCount;

	m_lighttraceShader.Activate();
	GL_CALL(glDispatchCompute, GetNumInitialLightSamples() * m_numRaysPerLightSample / m_localSizeLightPathtracer, 1, 1);

	GL_CALL(glMemoryBarrier, GL_SHADER_IMAGE_ACCESS_BARRIER_BIT); // Output texture
	GL_CALL(glMemoryBarrier, GL_SHADER_STORAGE_BARRIER_BIT); // Light Cache

	m_pathtraceShader.Activate();
	GL_CALL(glDispatchCompute, m_backbuffer->GetWidth() / m_localSizePathtracer.x, m_backbuffer->GetHeight() / m_localSizePathtracer.y, 1);


	PerIterationBufferUpdate();

	// Ensure that all future fetches will use the modified data.
	// See https://www.opengl.org/wiki/Memory_Model#Ensuring_visibility
	GL_CALL(glMemoryBarrier, GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
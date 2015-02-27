#include "bidirectionalpathtracer.hpp"
#include "renderersystem.hpp"
#include <glhelper/texture2d.hpp>
#include <glhelper/shaderstoragebufferview.hpp>
#include <glhelper/uniformbufferview.hpp>
#include "../utilities/flagoperators.hpp"
#include "../utilities/logger.hpp"

#include <iostream>

const unsigned int BidirectionalPathtracer::m_localSizeLightPathtracer = 8*8;
const ei::UVec2 BidirectionalPathtracer::m_localSizePathtracer = ei::UVec2(8, 8);

BidirectionalPathtracer::BidirectionalPathtracer(RendererSystem& _rendererSystem) :
	Renderer(_rendererSystem),
	m_lighttraceShader("lighttracer_bidir"),
	m_pathtraceShader("pathtracer_bidir"),
	m_warmupLighttraceShader("lighttracer_warmup_bidir"),
	m_needToDetermineNeededLightCacheCapacity(false),
	m_numRaysPerLightSample(0)
{
	std::string additionalDefines;
#ifdef SHOW_SPECIFIC_PATHLENGTH
	additionalDefines += "#define SHOW_SPECIFIC_PATHLENGTH " + std::to_string(SHOW_SPECIFIC_PATHLENGTH) + "\n";
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

	Assert(pathtrace_LightCacheCount.bufferBinding == lighttrace_LightCacheCount.bufferBinding &&
		pathtrace_LightCacheCount.bufferBinding == warmupLighttrace_LightCacheCount.bufferBinding &&
		pathtrace_LightCacheCount.bufferDataSizeByte == lighttrace_LightCacheCount.bufferDataSizeByte &&
		pathtrace_LightCacheCount.bufferDataSizeByte == warmupLighttrace_LightCacheCount.bufferDataSizeByte, "Different definitions of LightCacheCount SSBO within shaders!");

	auto pathtrace_LightCache = m_lighttraceShader.GetShaderStorageBufferInfo().find("LightCache")->second;
	auto lighttrace_LightCache = m_lighttraceShader.GetShaderStorageBufferInfo().find("LightCache")->second;
	auto warmupLighttrace_LightCache = m_warmupLighttraceShader.GetShaderStorageBufferInfo().find("LightCache")->second;

	Assert(pathtrace_LightCache.bufferBinding == lighttrace_LightCache.bufferBinding &&
		pathtrace_LightCache.bufferBinding == warmupLighttrace_LightCache.bufferBinding &&
		pathtrace_LightCache.bufferDataSizeByte == lighttrace_LightCache.bufferDataSizeByte &&
		pathtrace_LightCache.bufferDataSizeByte == warmupLighttrace_LightCache.bufferDataSizeByte, "Different definitions of LightCache SSBO within shaders!");
#endif


	m_lightpathtraceUBO = std::make_unique<gl::UniformBufferView>(m_lighttraceShader, "LightPathTrace");
	m_lightpathtraceUBO->BindBuffer(4);

	m_lightCacheFillCounter = std::make_unique<gl::ShaderStorageBufferView>(std::make_shared<gl::Buffer>(4, gl::Buffer::Usage::MAP_READ), "LightCacheCount");
	m_warmupLighttraceShader.BindSSBO(*m_lightCacheFillCounter);

	m_rendererSystem.SetNumInitialLightSamples(128);
}

void BidirectionalPathtracer::SetScreenSize(const gl::Texture2D& _newBackbuffer)
{
	_newBackbuffer.BindImage(0, gl::Texture::ImageAccess::READ_WRITE);

	// Lock texture for light-camera path connections.
	m_lockTexture.reset(new gl::Texture2D(_newBackbuffer.GetWidth(), _newBackbuffer.GetHeight(), gl::TextureFormat::R32UI));
	m_lockTexture->ClearToZero(0);
	m_lockTexture->BindImage(1, gl::Texture::ImageAccess::READ_WRITE);

	// Rule: Every block (size = m_localSizeLightPathtracer) should work with the same initial light sample!
	int numPixels = _newBackbuffer.GetWidth() * _newBackbuffer.GetHeight();
	m_numRaysPerLightSample = std::max(m_localSizeLightPathtracer, (numPixels / m_rendererSystem.GetNumInitialLightSamples() / m_localSizeLightPathtracer) * m_localSizeLightPathtracer);

	// Set constants ...
	m_lightpathtraceUBO->GetBuffer()->Map();
	(*m_lightpathtraceUBO)["NumRaysPerLightSample"].Set(static_cast<std::int32_t>(m_numRaysPerLightSample));
	(*m_lightpathtraceUBO)["LightRayPixelWeight"].Set(ei::PI * 2.0f / m_numRaysPerLightSample);
	(*m_lightpathtraceUBO)["LightCacheCapacity"].Set(std::numeric_limits<std::int32_t>().max()); // Not known yet.
	m_lightpathtraceUBO->GetBuffer()->Unmap();

	m_needToDetermineNeededLightCacheCapacity = true;
}

void BidirectionalPathtracer::CreateLightCacheWithCapacityEstimate()
{
	// Create SSBO for light path length sum.
	auto lightPathLengthBuffer = std::make_unique<gl::ShaderStorageBufferView>(std::make_shared<gl::Buffer>(4, gl::Buffer::Usage::MAP_READ), "LightPathLength");
	m_warmupLighttraceShader.BindSSBO(*lightPathLengthBuffer);

	// Perform Warmup to find out how large the lightcache should be.
	LOG_LVL2("Starting bpt warmup to estimate needed light cache capacity...");
	m_warmupLighttraceShader.Activate();
	static const unsigned int numWarmupRuns = 8;
	const unsigned int blockCountPerRun = m_rendererSystem.GetNumInitialLightSamples() * m_numRaysPerLightSample / m_localSizeLightPathtracer;
	std::uint64_t numCacheEntries = 0;
	std::uint64_t summedPathLength = 0;
	for (unsigned int run = 0; run < numWarmupRuns; ++run)
	{
		GL_CALL(glDispatchCompute, blockCountPerRun, 1, 1);
		GL_CALL(glMemoryBarrier, GL_SHADER_STORAGE_BARRIER_BIT);

		numCacheEntries += *reinterpret_cast<const std::uint32_t*>(m_lightCacheFillCounter->GetBuffer()->Map());
		m_lightCacheFillCounter->GetBuffer()->Unmap();

		summedPathLength += *reinterpret_cast<const std::uint32_t*>(lightPathLengthBuffer->GetBuffer()->Map());
		lightPathLengthBuffer->GetBuffer()->Unmap();

		// Driver workaround: keep in fast memory while computing
		// A map may move the memory block to slower memory where it stays.
		m_lightCacheFillCounter = std::make_unique<gl::ShaderStorageBufferView>(std::make_shared<gl::Buffer>(4, gl::Buffer::Usage::MAP_READ), "LightCacheCount");
		m_warmupLighttraceShader.BindSSBO(*m_lightCacheFillCounter);

		lightPathLengthBuffer = std::make_unique<gl::ShaderStorageBufferView>(std::make_shared<gl::Buffer>(4, gl::Buffer::Usage::MAP_READ), "LightPathLength");
		m_warmupLighttraceShader.BindSSBO(*lightPathLengthBuffer);

		// Important to change random seed
		m_rendererSystem.PerIterationBufferUpdate();
	}

	// Reset iteration count for actual rendering
	m_rendererSystem.ResetIterationCount();

	// Take mean and report to log.
	int averageLightPathLength = static_cast<int>(summedPathLength / numWarmupRuns / blockCountPerRun / m_localSizeLightPathtracer + 1); // Rounding up (for such large numbers this means +1 almost always)
	numCacheEntries /= numWarmupRuns;
	int lightCacheCapacity = static_cast<unsigned int>(numCacheEntries + numCacheEntries / 10); // Add 10% safety margin.
#ifdef SHOW_SPECIFIC_PATHLENGTH
	const unsigned int lightCacheEntrySize = sizeof(float) * 4 * 4 + sizeof(std::uint32_t);
#else
	const unsigned int lightCacheEntrySize = sizeof(float) * 4 * 4;
#endif
	unsigned int lightCacheSizeInBytes = lightCacheCapacity * lightCacheEntrySize;

	LOG_LVL2("... bpt warmup done. Reserving " << lightCacheSizeInBytes /1024/1024 << "mb light sample cache. Average light path length " << averageLightPathLength);

	m_lightCacheFillCounter = std::make_unique<gl::ShaderStorageBufferView>(std::make_shared<gl::Buffer>(4, gl::Buffer::Usage::IMMUTABLE), "LightCacheCount");
	m_warmupLighttraceShader.BindSSBO(*m_lightCacheFillCounter);

	// Create light cache
	m_lightpathtraceUBO->GetBuffer()->Map();
	(*m_lightpathtraceUBO)["LightCacheCapacity"].Set(lightCacheCapacity);
	(*m_lightpathtraceUBO)["AverageLightPathLength"].Set(averageLightPathLength);
	m_lightpathtraceUBO->GetBuffer()->Unmap();
	m_lightCache = std::make_unique<gl::ShaderStorageBufferView>(std::make_shared<gl::Buffer>(lightCacheSizeInBytes, gl::Buffer::Usage::IMMUTABLE), "LightCache");
	m_lighttraceShader.BindSSBO(*m_lightCache);
}

void BidirectionalPathtracer::Draw()
{
	if (m_needToDetermineNeededLightCacheCapacity)
	{
		CreateLightCacheWithCapacityEstimate();
		m_needToDetermineNeededLightCacheCapacity = false;
	}

	m_lighttraceShader.Activate();
	GL_CALL(glDispatchCompute, m_rendererSystem.GetNumInitialLightSamples() * m_numRaysPerLightSample / m_localSizeLightPathtracer, 1, 1);

	GL_CALL(glMemoryBarrier, GL_SHADER_IMAGE_ACCESS_BARRIER_BIT); // Output texture
	GL_CALL(glMemoryBarrier, GL_SHADER_STORAGE_BARRIER_BIT); // Light Cache

	m_pathtraceShader.Activate();
	GL_CALL(glDispatchCompute, m_rendererSystem.GetBackbuffer().GetWidth() / m_localSizePathtracer.x, m_rendererSystem.GetBackbuffer().GetHeight() / m_localSizePathtracer.y, 1);

	//m_rendererSystem.DispatchShowLightCacheShader();
}
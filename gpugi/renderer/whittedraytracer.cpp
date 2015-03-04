#include "whittedraytracer.hpp"
#include "renderersystem.hpp"
#include "debugrenderer/raytracemeshinfo.hpp"
#include "debugrenderer/hierarchyvisualization.hpp"
#include <glhelper/texture2d.hpp>

#include <fstream>

const ei::UVec2 WhittedRayTracer::m_localSizePathtracer = ei::UVec2(8, 8);

WhittedRayTracer::WhittedRayTracer(RendererSystem& _rendererSystem) :
	Renderer(_rendererSystem),
	m_pathtracerShader("raytracer")
{
	std::string additionalDefines = "#define STOP_ON_DIFFUSE_BOUNCE\n";
#ifdef SHOW_SPECIFIC_PATHLENGTH
	additionalDefines += "#define SHOW_SPECIFIC_PATHLENGTH " + std::to_string(SHOW_SPECIFIC_PATHLENGTH) + "\n";
#endif

	m_pathtracerShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/whittedraytracer.comp", additionalDefines);
	m_pathtracerShader.CreateProgram();

	m_rendererSystem.SetNumInitialLightSamples(128);
}

void WhittedRayTracer::SetScreenSize(const gl::Texture2D& _newBackbuffer)
{
	_newBackbuffer.BindImage(0, gl::Texture::ImageAccess::READ_WRITE);
}

void WhittedRayTracer::Draw()
{
	m_pathtracerShader.Activate();
	GL_CALL(glDispatchCompute, m_rendererSystem.GetBackbuffer().GetWidth() / m_localSizePathtracer.x, m_rendererSystem.GetBackbuffer().GetHeight() / m_localSizePathtracer.y, 1);

	//m_rendererSystem.DispatchShowLightCacheShader();
}
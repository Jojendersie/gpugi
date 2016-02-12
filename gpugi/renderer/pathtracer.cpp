#include "pathtracer.hpp"
#include "renderersystem.hpp"
#include "debugrenderer/raytracemeshinfo.hpp"
#include "debugrenderer/hierarchyvisualization.hpp"
#include "scene/scene.hpp"
#include <glhelper/texture2d.hpp>
#include <glhelper/texturecubemap.hpp>

#include <fstream>

const ei::UVec2 Pathtracer::m_localSizePathtracer = ei::UVec2(8, 8);

Pathtracer::Pathtracer(RendererSystem& _rendererSystem) :
	Renderer(_rendererSystem),
	m_pathtracerShader("pathtracer")
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

void Pathtracer::SetScene(std::shared_ptr<Scene> _scene)
{
	RecompileShaders(_scene->GetBvhTypeDefineString());
}

void Pathtracer::SetScreenSize(const gl::Texture2D& _newBackbuffer)
{
	_newBackbuffer.BindImage(0, gl::Texture::ImageAccess::READ_WRITE);
}

void Pathtracer::SetEnvironmentMap(std::shared_ptr<gl::TextureCubemap> _envMap)
{
	_envMap->Bind(12);
}

void Pathtracer::Draw()
{
	m_pathtracerShader.Activate();
	GL_CALL(glDispatchCompute, m_rendererSystem.GetBackbuffer().GetWidth() / m_localSizePathtracer.x, m_rendererSystem.GetBackbuffer().GetHeight() / m_localSizePathtracer.y, 1);

	//m_rendererSystem.DispatchShowLightCacheShader();
}

void Pathtracer::RecompileShaders(const std::string& _additionalDefines)
{
	std::string additionalDefines = _additionalDefines;
#ifdef SHOW_SPECIFIC_PATHLENGTH
	additionalDefines += "#define SHOW_SPECIFIC_PATHLENGTH " + std::to_string(SHOW_SPECIFIC_PATHLENGTH) + "\n";
#endif

	m_pathtracerShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "shader/pathtracer.comp", additionalDefines);
	m_pathtracerShader.CreateProgram();
}

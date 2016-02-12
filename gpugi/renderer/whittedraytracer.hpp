#pragma once

#include "renderer.hpp"
#include <glhelper/screenalignedtriangle.hpp>
#include <glhelper/framebufferobject.hpp>
#include <glhelper/shaderobject.hpp>

namespace gl
{
	class TextureBufferView;
}

class WhittedRayTracer : public Renderer
{
public:
	/// \param [in] _whitted Stop on first diffuse hit.
	WhittedRayTracer(RendererSystem& _rendererSystem);

	std::string GetName() const override { return "WPT"; }

	void SetScene(std::shared_ptr<Scene> _scene) override;
	void SetScreenSize(const gl::Texture2D& _newBackbuffer) override;

	void Draw() override;

private:
	gl::ShaderObject m_pathtracerShader;
	static const ei::UVec2 m_localSizePathtracer;

	void RecompileShaders(const std::string& _additionalDefines);
};

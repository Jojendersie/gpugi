#pragma once

#include "renderer.hpp"
#include <glhelper/screenalignedtriangle.hpp>
#include <glhelper/framebufferobject.hpp>
#include <glhelper/shaderobject.hpp>

namespace gl
{
	class TextureBufferView;
}

class Pathtracer : public Renderer
{
public:
	/// \param [in] _whitted Stop on first diffuse hit.
	Pathtracer(RendererSystem& _rendererSystem);

	std::string GetName() const override { return "PT"; }

	void SetScreenSize(const gl::Texture2D& _newBackbuffer) override;
	void SetEnvironmentMap(std::shared_ptr<gl::TextureCubemap> _envMap) override;

	void Draw() override;

private:
	gl::ShaderObject m_pathtracerShader;
	static const ei::UVec2 m_localSizePathtracer;
};

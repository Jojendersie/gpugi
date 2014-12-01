#pragma once

#include "renderer.hpp"
#include "../glhelper/screenalignedtriangle.hpp"
#include "../glhelper/framebufferobject.hpp"

namespace gl
{
	class TextureBufferView;
}

class Pathtracer : public Renderer
{
public:
	Pathtracer();

	std::string GetName() const override { return "PT"; }

	void SetScreenSize(const ei::IVec2& newSize) override;

	void Draw() override;

private:
	gl::ShaderObject m_pathtracerShader;
	static const ei::UVec2 m_localSizePathtracer;
};


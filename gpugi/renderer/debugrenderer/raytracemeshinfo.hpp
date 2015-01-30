#pragma once

#include "DebugRenderer.hpp"
#include <glhelper/shaderobject.hpp>
#include <memory>

namespace gl
{
	class ScreenAlignedTriangle;
}

/// Debug renderer for visualizing various mesh properties via raytracing (zero bounce).
///
/// Relies on Renderer's standard UBO and TBO contents and bindings.
class RaytraceMeshInfo : public DebugRenderer
{
public:
	RaytraceMeshInfo(const Renderer& _parentRenderer);
	~RaytraceMeshInfo();

	static const std::string Name;
	std::string GetName() const override { return Name; }
	void Draw() override;

private:
	std::unique_ptr<gl::UniformBufferView> m_settingsUBO;
	std::unique_ptr<gl::ScreenAlignedTriangle> m_screenTri;
	gl::ShaderObject m_infoShader;

	static const ei::UVec2 m_localSize;
};


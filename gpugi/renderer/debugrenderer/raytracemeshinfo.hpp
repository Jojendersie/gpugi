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
	RaytraceMeshInfo(Renderer& _parentRenderer);
	~RaytraceMeshInfo();

	void SetScene(std::shared_ptr<Scene> _scene) override;
	static const std::string Name;
	std::string GetName() const override { return Name; }
	void Draw() override;

private:
	void RecompileShaders(const std::string& _additionalDefines);
	std::unique_ptr<gl::Buffer> m_settingsUBO;
	std::unique_ptr<gl::ScreenAlignedTriangle> m_screenTri;
	gl::ShaderObject m_infoShader;
};


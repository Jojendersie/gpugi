#pragma once

#include "DebugRenderer.hpp"
#include <glhelper/shaderobject.hpp>
#include <memory>

namespace gl
{
	class ScreenAlignedTriangle;
}

/// Debug renderer for visualizing the triangle/node importance of importance rendering.
///
/// Relies on Hierarchy Importance Renderer's standard UBO and TBO contents and bindings.
class ImportanceVisualization : public DebugRenderer
{
public:
	ImportanceVisualization(Renderer& _parentRenderer);
	~ImportanceVisualization();

	void SetScene(std::shared_ptr<Scene> _scene) override;
	static const std::string Name;
	std::string GetName() const override { return Name; }
	void Draw() override;

private:
	//std::unique_ptr<gl::Buffer> m_settingsUBO;
	std::unique_ptr<gl::ScreenAlignedTriangle> m_screenTri;
	gl::ShaderObject m_infoShader;

	void RecompileShaders(const std::string& _additionalDefines);
};


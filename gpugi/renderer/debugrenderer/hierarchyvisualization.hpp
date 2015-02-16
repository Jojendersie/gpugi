#pragma once

#include "DebugRenderer.hpp"
#include <glhelper/shaderobject.hpp>
#include <memory>

namespace gl
{
	class ScreenAlignedTriangle;
	class Buffer;
	class VertexArrayObject;
}

/// Debug renderer for visualizing the scene hierarchy.
///
/// Coloring is normally depended on the relative depth of the nodes.
/// If the active renderer is the HierarchyImportance Renderer, the color is defined by the node's importance value.
class HierarchyVisualization : public DebugRenderer
{
public:
	HierarchyVisualization(Renderer& _parentRenderer);
	~HierarchyVisualization();

	void SetScene(std::shared_ptr<Scene> _scene) override;

	static const std::string Name;
	std::string GetName() const override { return Name; }
	void Draw() override;

private:
	struct Instance
	{
		std::uint32_t nodeIndex;
		std::uint32_t depth;
	};

	std::unique_ptr<gl::UniformBufferView> m_settingsUBO;

	std::unique_ptr<gl::VertexArrayObject> m_boxVAO;
	
	std::unique_ptr<gl::Buffer> m_boxVertexBuffer;
	std::unique_ptr<gl::Buffer> m_boxSolidIndices;
	std::unique_ptr<gl::Buffer> m_boxLineIndices;

	std::unique_ptr<gl::Buffer> m_instanceBuffer;

	gl::ShaderObject m_shader;

	/// Number of instances that come before level i is given by m_hierachyLevelOffset[i].
	/// The last element is the total number of inner nodes to make computing the number of elements per level easier.
	std::vector<unsigned int> m_hierachyLevelOffset;
};


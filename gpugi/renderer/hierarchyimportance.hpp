#pragma once

#include "renderer.hpp"
#include <glhelper/screenalignedtriangle.hpp>
#include <glhelper/framebufferobject.hpp>
#include <glhelper/shaderobject.hpp>
#include "../scene/lightsampler.hpp"

namespace gl
{
	class TextureBufferView;
	class Texture2D;
}

/// Renderer that generates an importance value for each node/triangle.
class HierarchyImportance : public Renderer
{
public:
	HierarchyImportance(RendererSystem& _rendererSystem);

	std::string GetName() const override { return "HierachyImp"; }

	void SetScene(std::shared_ptr<Scene> _scene) override;
	void SetScreenSize(const gl::Texture2D& _newBackbuffer) override;

	/// Returns hierarchy importance for possible use in other systems.
	///
	/// Contains a float entry for all inner nodes (first) and triangles (after node entries).
	/// If no call of UpdateHierarchyNodeImportance proceeded, only the triangles have valid importance values.
	std::shared_ptr<gl::Buffer>& GetHierachyImportance() { return m_hierarchyImportance; }

	/// Updates the hierarchy importance of all inner nodes by propagating them from the triangles up through the tree.
	void UpdateHierarchyNodeImportance();

	void Draw() override;

	/// Internal ssbo binding point of the hierarchy importance buffer.
	static const unsigned int s_hierarchyImportanceBinding = 0;

private:
	gl::ShaderObject m_hierarchyImpAcquisitionShader;
	gl::ShaderObject m_hierarchyImpTriagVisShader;
	gl::ShaderObject m_hierarchyImpPropagationInitShader;
	gl::ShaderObject m_hierarchyImpPropagationNodeShader;

	gl::UniformBufferMetaInfo m_hierarchyImportanceUBOInfo;
	std::unique_ptr<gl::Buffer> m_hierarchyImportanceUBO;
	static const ei::UVec2 m_localSizePathtracer;

	std::unique_ptr<gl::TextureBufferView> m_sceneParentPointer;
	std::shared_ptr<gl::Buffer> m_hierarchyImportance;
};


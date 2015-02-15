#pragma once

#include "renderer.hpp"
#include <glhelper/screenalignedtriangle.hpp>
#include <glhelper/framebufferobject.hpp>
#include <glhelper/shaderobject.hpp>
#include "../scene/lighttrianglesampler.hpp"

namespace gl
{
	class UniformBufferView;
	class ShaderStorageBufferView;
	class TextureBufferView;
	class Texture2D;
}

class HierarchyImportance : public Renderer
{
public:
	HierarchyImportance(RendererSystem& _rendererSystem);

	std::string GetName() const override { return "HierachyImp"; }

	void SetScene(std::shared_ptr<Scene> _scene) override;
	void SetScreenSize(const gl::Texture2D& _newBackbuffer) override;

	/// Returns hierarchy importance for possible use in other systems.
	///
	/// Hierarchy importance is saved a texture for practical reasons:
	/// Using a shader storage buffer with a single float array fails to compile on NVIDIA driver (15.02.2015) since its elements are collapsed to vec4 for which no atomic operation is available.
	/// Texture buffers on the other hand do not support atomic operatoins at all.
	std::shared_ptr<gl::Texture2D> GetHierachyImportance();

	void Draw() override;

private:
	gl::ShaderObject m_hierarchyImpAcquisitionShader;
	gl::ShaderObject m_hierarchyImpTriagVisShader;
	gl::ShaderObject m_hierarchyImpPropagationShader;

	std::unique_ptr<gl::UniformBufferView> m_hierarchyImportanceUBO;
	static const ei::UVec2 m_localSizePathtracer;

	std::unique_ptr<gl::TextureBufferView> m_sceneParentPointer;
	std::shared_ptr<gl::ShaderStorageBufferView> m_hierarchyImportance;
};


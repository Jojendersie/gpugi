#pragma once

#include "../glhelper/shaderobject.hpp"
#include "../glhelper/uniformbuffer.hpp"

#include <memory>
#include <ei/matrix.hpp>

namespace gl
{
	class Texture2D;
}
class Camera;
class Scene;

class Renderer
{
public:
	virtual ~Renderer();

	virtual void SetScene(std::shared_ptr<Scene> _scene) = 0;
	/// Sets camera. May result in a scene reset!
	virtual void SetCamera(const Camera& camera);
	virtual void SetScreenSize(const ei::IVec2& newSize);



	gl::Texture2D& GetBackbuffer() { return *m_backbuffer; }

	virtual std::string GetName() const = 0;

	/// Performs a draw iteration.
	virtual void Draw() = 0;

	/// Hoy many iterations did the current image already render.
	/// Meaning may differ with concrete renderer.
	unsigned long GetIterationCount() const { return m_iterationCount;  }

protected:
	void InitStandardUBOs(const gl::ShaderObject& _reflectionShader);

	Renderer();

	std::unique_ptr<gl::Texture2D> m_backbuffer;
	unsigned long m_iterationCount;

	gl::UniformBufferView m_globalConstUBO;
	gl::UniformBufferView m_cameraUBO;
};


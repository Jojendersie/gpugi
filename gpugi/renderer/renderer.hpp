#pragma once

#include <memory>
#include <string>

// Define this macro to display only the paths of the given length (with 1 = direct lighting)
// Will alter shader code with the same macro automatically.
// This is independently implemented in Pathtracer, BidirectionalPathtracer, Lighttracer and Hierachyimportance.
//#define SHOW_SPECIFIC_PATHLENGTH 2


namespace gl
{
	class Texture2D;
	class TextureCubemap;
}

class Scene;
class Camera;
class RendererSystem;

/// Abstract base class for all renderer.
///
/// Provides a unified interface for interop with RendererSystem.
class Renderer
{
public:
	virtual ~Renderer() = default;

	/// Returns a unique name for this renderer type.
	virtual std::string GetName() const = 0;

	/// Sets scene.
	virtual void SetScene(std::shared_ptr<Scene> _scene) {}
	/// Sets camera.
	virtual void SetCamera(const Camera& camera) {}
	/// Sets back buffer size.
	virtual void SetScreenSize(const gl::Texture2D& _newBackbuffer) = 0;
	/// Bind the environment map if used
	virtual void SetEnvironmentMap(std::shared_ptr<gl::TextureCubemap> _envMap) {}
	/// Draw command, increments usually iteration count.
	virtual void Draw() = 0;

	/// Returns associated renderersystem.
	const RendererSystem& GetRendererSystem() const		{ return m_rendererSystem; }

protected:
	Renderer(RendererSystem& _rendererSystem) : m_rendererSystem(_rendererSystem) {}

	RendererSystem& m_rendererSystem;
};
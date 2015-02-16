#pragma once

#include <memory>
#include <string>

// Define this macro to display only the paths of the given length (with 1 = direct lighting)
// Will alter shader code with the same macro automatically.
// This is independently implemented in 
//#define SHOW_SPECIFIC_PATHLENGTH 1


namespace gl
{
	class Texture2D;
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
	/// Returns a unique name for this renderer type.
	virtual std::string GetName() const = 0;

	/// Sets scene.
	virtual void SetScene(std::shared_ptr<Scene> _scene) {}
	/// Sets camera.
	virtual void SetCamera(const Camera& camera) {}
	/// Sets back buffer size.
	virtual void SetScreenSize(const gl::Texture2D& _newBackbuffer) = 0;
	/// Draw command, increments usually iteration count.
	virtual void Draw() = 0;

	/// Returns associated renderersystem.
	const RendererSystem& GetRendererSystem() const		{ return m_rendererSystem; }

protected:
	Renderer(RendererSystem& _rendererSystem) : m_rendererSystem(_rendererSystem) {}

	RendererSystem& m_rendererSystem;
};
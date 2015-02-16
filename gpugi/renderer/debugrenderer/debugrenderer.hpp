#pragma once

#include <memory>
#include <string>

class Scene;
class Renderer;

/// Base class for all debug renderers.
///
/// Debug renderers behave similar to normal renders. However they can only be active while a "normal" renderer is set.
class DebugRenderer
{
public:
	DebugRenderer(Renderer& _parentRenderer) : m_parentRenderer(_parentRenderer) {}
	virtual ~DebugRenderer() {}

	/// Sets the scene for the debug renderer.
	///
	/// If a scene was already loaded into the Renderer, this function will be called right after the init.
	/// Of course it will also be called with every call of Renderer::SetScene
	virtual void SetScene(std::shared_ptr<Scene> _scene) {}

	virtual std::string GetName() const = 0;
	virtual void Draw() = 0;

protected:
	Renderer& m_parentRenderer;
};
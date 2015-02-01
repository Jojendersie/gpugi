#pragma once

#include <memory>
#include <string>

class Renderer;
class Scene;

/// Base class for all debug renderers.
///
/// A debug renderer is a part of renderer (see Renderer class). At any time there can only be one active debug renderer.
/// However a debug renderer may have suboptions (see globalconfig help).
/// Different renderers may share types of possible debug renderers, but are not required to do so.
/// There may be debug renderers that are only available for specific renderers.
/// Switching to a debug renderer should not change the state of the active renderer in any way, except the backbuffer.
/// If not documented otherwise, debug renderers will render directly to the hardware backbuffer. Screenshots may therefore still display the last "normal" frame.
class DebugRenderer
{
public:
	DebugRenderer(const Renderer& parentRenderer) : m_parentRenderer(parentRenderer) {}
	virtual ~DebugRenderer() {}

	/// Sets the scene for the debug renderer.
	///
	/// If a scene was already loaded into the Renderer, this function will be called right after the init.
	/// Of course it will also be called with every call of Renderer::SetScene
	virtual void SetScene(std::shared_ptr<Scene> _scene) {}

	virtual std::string GetName() const = 0;
	virtual void Draw() = 0;

protected:
	const Renderer& m_parentRenderer;
};
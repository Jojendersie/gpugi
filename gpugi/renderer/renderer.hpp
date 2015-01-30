#pragma once

#include <glhelper/shaderobject.hpp>
#include <glhelper/uniformbuffer.hpp>

#include <memory>
#include <ei/matrix.hpp>

#include "../scene/lighttrianglesampler.hpp"

// Define this macro to display only the paths of the given length (with 1 = direct lighting)
// Will alter shader code with the same macro automatically.
// This is independently implemented in 
//#define SHOW_SPECIFIC_PATHLENGTH 1

namespace gl
{
	class Texture2D;
	class TextureBufferView;
}
class Camera;
class Scene;
class DebugRenderer;

/// Base class for all renderer.
///
/// Provides a unified interface and implements various shared functions.
class Renderer
{
public:
	virtual ~Renderer();

	// The following functions may (depending on the renderer) reset the backbuffer and the iteration count to zero.

	/// Sets scene.
	virtual void SetScene(std::shared_ptr<Scene> _scene);
	/// Sets camera.
	virtual void SetCamera(const Camera& camera);
	/// Sets back buffer size.
	virtual void SetScreenSize(const ei::IVec2& newSize);

	// ---------

	gl::Texture2D& GetBackbuffer() { return *m_backbuffer; }

	virtual std::string GetName() const = 0;

	/// Performs a draw iteration.
	virtual void Draw() = 0;

	/// Returns if a debug renderer is active
	bool IsDebugRendererActive() { return m_activeDebugRenderer.get() != nullptr; }

	/// Hoy many iterations did the current image already render.
	/// Meaning may differ with concrete renderer.
	unsigned long GetIterationCount() const { return m_iterationCount;  }

	/// Registers debug renderer state config options at globalconfig.
	void RegisterDebugRenderStateConfigOptions();

protected:

	/// Defines default texture buffer binding assignment.
	enum class TextureBufferBindings
	{
		// Don't use 0, since this is reserved for changing purposes.
		TRIANGLES = 1,
		VERTEX_POSITIONS = 2,
		VERTEX_INFO = 3,
		HIERARCHY = 4,
		INITIAL_LIGHTSAMPLES = 5
	};

	/// Defines default constant buffer binding assignment.
	enum class UniformBufferBindings
	{
		GLOBALCONST = 0,
		CAMERA = 1,
		PERITERATION = 2,
		MATERIAL = 3,
	};

	void InitStandardUBOs(const gl::ShaderObject& _reflectionShader);
	virtual void PerIterationBufferUpdate();

	void SetNumInitialLightSamples(unsigned int _numInitialLightSamples);
	unsigned int GetNumInitialLightSamples() const { return m_numInitialLightSamples; }

	Renderer();

	std::unique_ptr<gl::Texture2D> m_backbuffer;

	/// Is reset for SetScene/Camera/Screensize. Other than that its handling is the task of the concrete renderer-impl.
	unsigned long m_iterationCount;

	/// Scene data
	std::shared_ptr<Scene> m_scene;

	/// List of all available debug renderer instantiation functions.
	std::vector<std::pair<std::function<std::unique_ptr<DebugRenderer>()>, std::string>> m_debugRenderConstructors;

	/// Active debug renderer. nullptr if none is active.
	std::unique_ptr<DebugRenderer> m_activeDebugRenderer;

private:
	void UpdateGlobalConstUBO();

	std::unique_ptr<gl::TextureBufferView> m_hierarchyBuffer;
	std::unique_ptr<gl::TextureBufferView> m_vertexPositionBuffer;
	std::unique_ptr<gl::TextureBufferView> m_vertexInfoBuffer;
	std::unique_ptr<gl::TextureBufferView> m_triangleBuffer;

	gl::UniformBufferView m_globalConstUBO;
	gl::UniformBufferView m_cameraUBO;
	gl::UniformBufferView m_materialUBO;
	gl::UniformBufferView m_perIterationUBO;


	unsigned int m_numInitialLightSamples;
	std::unique_ptr<gl::TextureBufferView> m_initialLightSampleBuffer;
	LightTriangleSampler m_lightTriangleSampler;
};


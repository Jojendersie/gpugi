#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <ei/vector.hpp>

#include "../camera/camera.hpp"
#include "../scene/lighttrianglesampler.hpp"

namespace gl
{
	class Texture2D;
	class TextureBufferView;
	class UniformBufferView;
	class ShaderObject;
}
class Scene;
class DebugRenderer;
class Renderer;

/// Administration system and framework for concrete (debug)renders.
///
/// Provides various shared resources for all renderers. There can not be more than one renderer and debugrenderer.
/// A debugrenderer requires an active renderer.
class RendererSystem
{
public:
	RendererSystem();
	~RendererSystem();

	/// Sets scene.
	///
	/// Active (debug)renderer will be notified.
	/// Resets iteration count.
	void SetScene(std::shared_ptr<Scene> _scene);

	/// Returns currently set scene.
	const std::shared_ptr<Scene>& GetScene() const { return m_scene; }


	/// Sets camera.
	///
	/// Active renderer will be notified.
	/// Resets iteration count.
	void SetCamera(const Camera& camera);


	/// Sets back buffer size.
	///
	/// Active renderer will be notified.
	/// Resets iteration count.
	void SetScreenSize(const ei::IVec2& newSize);

	/// Returns internal HDR backbuffer.
	gl::Texture2D& GetBackbuffer() { return *m_backbuffer; }



	/// Returns currently active renderer.
	Renderer* GetActiveRenderer() { return m_activeRenderer; }

	/// Destroys old renderer, creates new renderer and makes it active.
	///
	/// If there was a debug renderer active, it will be destroyed.
	template<typename RendererType, typename... Args>
	void SetRenderer(Args&&... _arguments)
	{
		DisableDebugRenderer();
		delete m_activeRenderer;
		ResetIterationCount();
		m_backbuffer->ClearToZero();

		m_activeRenderer = new RendererType(*this, _arguments...);
		m_activeRenderer->SetScreenSize(*m_backbuffer);
		m_activeRenderer->SetCamera(m_camera);
		m_activeRenderer->SetScene(m_scene);
	}

	/// Destroys old debug-renderer, creates new debug-renderer and makes it active.
	///
	/// Will be ignored if there is no active renderer.
	template<typename DebugRendererType, typename... Args>
	void SetDebugRenderer(Args&&... _arguments)
	{
		if (m_activeRenderer)
		{
			DisableDebugRenderer();

			m_activeDebugRenderer = new DebugRendererType(*m_activeRenderer, _arguments...);
			m_activeDebugRenderer->SetScene(m_scene);
		}
	}

	/// Destroys active debug renderer.
	void DisableDebugRenderer();

	/// Performs a draw iteration.
	void Draw();

	/// Returns if a debug renderer is active
	bool IsDebugRendererActive() { return m_activeDebugRenderer != nullptr; }

	/// Hoy many iterations did the current image already render.
	/// Meaning may differ with concrete renderer.
	unsigned long GetIterationCount() const { return m_iterationCount;  }


	void SetNumInitialLightSamples(unsigned int _numInitialLightSamples);
	unsigned int GetNumInitialLightSamples() const { return m_numInitialLightSamples; }

	void ResetIterationCount() { m_iterationCount = 0; PerIterationBufferUpdate(true); }

	/// Updates global per iteration uniform buffer.
	/// Will be called automatically after each draw.
	/// \param _iterationIncrement
	///		By default the iteration counter will be incremented. Specify false to suppress.
	void PerIterationBufferUpdate(bool _iterationIncrement = true);

private:

	void InitStandardUBOs(const gl::ShaderObject& _reflectionShader);

	void UpdateGlobalConstUBO();


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

	std::unique_ptr<gl::Texture2D> m_backbuffer;
	Camera m_camera;

	/// Is reset for SetScene/Camera/Screensize. Other than that its handling is the task of the concrete renderer-impl.
	unsigned long m_iterationCount;

	/// Scene data
	std::shared_ptr<Scene> m_scene;

	/// List of all available debug renderer instantiation functions.
	std::vector<std::pair<std::function<std::unique_ptr<DebugRenderer>()>, std::string>> m_debugRenderConstructors;

	std::unique_ptr<gl::TextureBufferView> m_hierarchyBuffer;
	std::unique_ptr<gl::TextureBufferView> m_vertexPositionBuffer;
	std::unique_ptr<gl::TextureBufferView> m_vertexInfoBuffer;
	std::unique_ptr<gl::TextureBufferView> m_triangleBuffer;

	std::unique_ptr<gl::UniformBufferView> m_globalConstUBO;
	std::unique_ptr<gl::UniformBufferView> m_cameraUBO;
	std::unique_ptr<gl::UniformBufferView> m_materialUBO;
	std::unique_ptr<gl::UniformBufferView> m_perIterationUBO;


	unsigned int m_numInitialLightSamples;
	std::unique_ptr<gl::TextureBufferView> m_initialLightSampleBuffer;
	LightTriangleSampler m_lightTriangleSampler;

	Renderer* m_activeRenderer;
	DebugRenderer* m_activeDebugRenderer;
};


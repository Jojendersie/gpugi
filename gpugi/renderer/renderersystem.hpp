#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <ei/vector.hpp>
#include <glhelper/shaderobject.hpp>

#include "../camera/camera.hpp"
#include "../scene/lightsampler.hpp"

namespace gl
{
	class Texture2D;
	class TextureBufferView;
	class TextureCubemap;
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

	/// Replaces the cubemap for the sky
	void SetEnvironmentMap(int _size, const std::string& _xneg, const std::string& _xpos,
		const std::string& _yneg, const std::string& _ypos,
		const std::string& _zneg, const std::string& _zpos);


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
	const gl::Texture2D& GetBackbuffer() const { return *m_backbuffer; }



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
		m_activeRenderer->SetEnvironmentMap(m_envMap);
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

	/// How many iterations did the current image already render.
	/// Meaning may differ with concrete renderer.
	unsigned long GetIterationCount() const { return m_iterationCount;  }

	/// Duration of all passed iterations (time for GetIterationCount()) in milliseconds
	uint64 GetRenderTime() const { return m_renderTime; }

	/// Configures the number of so called initial light samples
	///
	/// Creates a new light sample buffer which is bound to texture slot TextureBufferBindings::INITIAL_LIGHTSAMPLES.
	/// \attention Before this call the internal light samples buffer is not set.
	/// \see GetNumInitialLightSamples
	void SetNumInitialLightSamples(unsigned int _numInitialLightSamples);

	/// Returns number of initial light samples.
	/// \see SetNumInitialLightSamples
	unsigned int GetNumInitialLightSamples() const { return m_numInitialLightSamples; }
	
	/// Resets the iteration count to zero and updates the UBO.
	///
	/// Does not perform the other operations associated with PerIterationBufferUpdate.
	void ResetIterationCount();

	/// Updates global per iteration uniform buffer and initial light samples buffer.
	/// 
	/// Will be called automatically after each draw.
	/// Generates GetNumInitialLightSamples() new light samples.
	/// \param _iterationIncrement
	///		By default the iteration counter will be incremented. Specify false to suppress.
	void PerIterationBufferUpdate(bool _iterationIncrement = true);

	/// Visualize primary light cache via reprojection
	void DispatchShowLightCacheShader();

private:

	void InitStandardUBOs(const gl::ShaderObject& _reflectionShader);

	void UpdateGlobalConstUBO();

	void RecompileShaders(const std::string& _additionalDefines);


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
	uint64 m_renderTime;	// Time in ms

	/// Scene data
	std::shared_ptr<Scene> m_scene;
	std::shared_ptr<gl::TextureCubemap> m_envMap;

	/// List of all available debug renderer instantiation functions.
	std::vector<std::pair<std::function<std::unique_ptr<DebugRenderer>()>, std::string>> m_debugRenderConstructors;

	std::unique_ptr<gl::TextureBufferView> m_hierarchyBuffer;
	std::unique_ptr<gl::TextureBufferView> m_vertexPositionBuffer;
	std::unique_ptr<gl::TextureBufferView> m_vertexInfoBuffer;
	std::unique_ptr<gl::TextureBufferView> m_triangleBuffer;

	std::unique_ptr<gl::Buffer> m_globalConstUBO;
	gl::UniformBufferMetaInfo m_globalConstUBOInfo;
	std::unique_ptr<gl::Buffer> m_cameraUBO;
	gl::UniformBufferMetaInfo m_cameraUBOInfo;
	std::unique_ptr<gl::Buffer> m_materialUBO;
	gl::UniformBufferMetaInfo m_materialUBOInfo;
	std::unique_ptr<gl::Buffer> m_perIterationUBO;
	gl::UniformBufferMetaInfo m_perIterationUBOInfo;


	unsigned int m_numInitialLightSamples;
	std::unique_ptr<gl::TextureBufferView> m_initialLightSampleBuffer;
	LightSampler m_lightSampler;

	/// Project light caches to screen. Can be used from any renderer if the
	/// light <-> eye path is missing. Invoke once per light cache.
	gl::ShaderObject m_showLightCachesShader;

	Renderer* m_activeRenderer;
	DebugRenderer* m_activeDebugRenderer;
};


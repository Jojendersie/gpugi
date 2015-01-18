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

/// Base class for all renderer.
///
/// Provides a unified interface and implements various shared functions.
class Renderer
{
public:
	virtual ~Renderer();

	// All following functions reset the backbuffer and the iteration count to zero.

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

	/// Hoy many iterations did the current image already render.
	/// Meaning may differ with concrete renderer.
	unsigned long GetIterationCount() const { return m_iterationCount;  }

protected:

	enum class TextureBufferBindings
	{
		// Don't use 0, since this is reserved for changing purposes.
		TRIANGLES = 1,
		VERTEX_POSITIONS = 2,
		VERTEX_INFO = 3,
		HIERARCHY = 4,
		INITIAL_LIGHTSAMPLES = 5
	};

	enum class UniformBufferBindings
	{
		GLOBALCONST = 0,
		CAMERA = 1,
		PERITERATION = 2,
		MATERIAL = 3
	};

	void InitStandardUBOs(const gl::ShaderObject& _reflectionShader);
	virtual void PerIterationBufferUpdate();

	void SetNumInitialLightSamples(unsigned int _numInitialLightSamples);
	unsigned int GetNumInitialLightSamples() const { return m_numInitialLightSamples; }

	Renderer();

	std::unique_ptr<gl::Texture2D> m_backbuffer;

	/// Is reset for SetScene/Camera/Screensize. Other than that its handling is the task of the concrete renderer-impl.
	unsigned long m_iterationCount;

	// Scene data
	std::shared_ptr<Scene> m_scene;


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


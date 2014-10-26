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

class Renderer
{
public:
	virtual ~Renderer();

	gl::Texture2D& GetBackbuffer() { return *m_backbuffer; }

	/// Sets camera. May result in a scene reset!
	virtual void SetCamera(const Camera& camera)=0;

	/// Performs a draw iteration.
	virtual void Draw() = 0;

protected:
	virtual void OnResize(const ei::UVec2& newSize);

	Renderer();

	void CreateBackbuffer(const ei::UVec2& resolution);

	std::unique_ptr<gl::Texture2D> m_backbuffer;
};


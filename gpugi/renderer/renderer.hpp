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

	gl::Texture2D& GetBackbuffer() { return *backbuffer; }

	/// Sets camera. May result in a scene reset!
	virtual void SetCamera(const Camera& camera)=0;

	/// Performs a draw iteration.
	virtual void Draw() = 0;

protected:
	Renderer();

	void CreateBackbuffer(const ei::IVec2& resolution);

	std::unique_ptr<gl::Texture2D> backbuffer;
};


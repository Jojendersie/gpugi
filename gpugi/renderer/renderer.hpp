#pragma once

#include "../glhelper/shaderobject.hpp"
#include "../glhelper/uniformbuffer.hpp"

#include <memory>
#include <ei/matrix.hpp>

namespace gl
{
	class Texture2D;
}

class Renderer
{
public:
	virtual ~Renderer();

	gl::Texture2D& GetBackbuffer() { return *backbuffer; }

	/// Performs a draw iteration. Todo: Communicate camera etc.
	virtual void Draw() = 0;

protected:
	Renderer();

	void CreateBackbuffer(const ei::IVec2& resolution);

	std::unique_ptr<gl::Texture2D> backbuffer;
};


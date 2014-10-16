#pragma once

#include "../glhelper/texture2d.hpp"
#include "../glhelper/shaderobject.hpp"
#include "../glhelper/uniformbuffer.hpp"

class Renderer
{
public:
	virtual ~Renderer();

	gl::Texture2D& GetBackbuffer() { return backbuffer; }

	/// Performs a draw iteration. Todo: Communicate camera etc.
	virtual void Draw() = 0;

protected:
	Renderer();

	gl::Texture2D backbuffer;
};


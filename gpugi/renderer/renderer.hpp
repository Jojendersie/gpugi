#pragma once

#include "../glhelper/texture2d.hpp"
#include "../glhelper/shaderobject.hpp"

class Renderer
{
public:
	virtual ~Renderer();

	gl::Texture2D& GetBackbuffer() { return backbuffer; }

	/// Performs a draw iteration. Todo: Communicate camera etc.
	void Draw();

protected:
	Renderer();

private:
	gl::Texture2D backbuffer;
};


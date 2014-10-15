#pragma once

#include "gl.hpp"

namespace gl
{
	// class for rendering a screen aligned triangle
	class ScreenAlignedTriangle
	{
	public:
		ScreenAlignedTriangle();
		~ScreenAlignedTriangle();

		void Draw();

	private:
		BufferId vbo;
	};
}
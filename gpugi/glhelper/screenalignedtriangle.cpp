#include "screenalignedtriangle.hpp"

namespace gl
{

	struct ScreenTriVertex
	{
		float position[2];
	};

	ScreenAlignedTriangle::ScreenAlignedTriangle()
	{
		ScreenTriVertex screenTriangle[3];
		screenTriangle[0].position[0] = -1.0f;
		screenTriangle[0].position[1] = -1.0f;
		screenTriangle[1].position[0] = 3.0f;
		screenTriangle[1].position[1] = -1.0f;
		screenTriangle[2].position[0] = -1.0f;
		screenTriangle[2].position[1] = 3.0f;

		GL_CALL(glCreateBuffers, 1, &vbo);
		GL_CALL(glNamedBufferStorage, vbo, sizeof(screenTriangle), screenTriangle, 0);
	}


	ScreenAlignedTriangle::~ScreenAlignedTriangle(void)
	{
		GL_CALL(glDeleteBuffers, 1, &vbo);
	}

	void ScreenAlignedTriangle::Draw()
	{
		GL_CALL(glBindBuffer, GL_ARRAY_BUFFER, vbo);
		GL_CALL(glVertexAttribPointer, 0, 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(float) * 2), nullptr);
		GL_CALL(glEnableVertexAttribArray, 0);

		GL_CALL(glDrawArrays, GL_TRIANGLES, 0, 3);
	}

}
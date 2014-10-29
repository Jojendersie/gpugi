#pragma once

#include "ei/matrix.hpp"
#include "../glhelper/buffer.hpp"
#include "../../bvhmake/filedef.hpp"

#include <string>
#include <memory>

/// Scene manager - loads data from a file and pushes it to the GPU.
class Scene
{
public:
    /// Load a scene.
    Scene( const std::string& _file );

    /// Unload all the scene data and GPU resources
    ~Scene();

	/// Inner node struct on GPU side
	struct TreeNode
	{
		uint32 firstChild;	///< Index of the first child, others are reached by the child's escape. 0 is the invalid index.
		uint32 escape;		///< Index of the next node when this completed. 0 is the invalid index.
		// TODO the bounding volume should be part of this node
	};

	/// "Index buffer content": UVec4 with 3 vertex indices and a material index
	struct Triangle
	{
		uint32 vertices[3];
		uint32 material;
	};

	/// Stored vertex data in file and GPU memory
	struct Vertex
	{
		ε::Vec3 position;
        ε::Vec3 normal;
        ε::Vec2 texcoord;
	};

	std::shared_ptr<gl::Buffer> GetVertexBuffer() const { return m_vertexBuffer; }
	std::shared_ptr<gl::Buffer> GetTriangleBuffer() const { return m_triangleBuffer; }
private:
	std::shared_ptr<gl::Buffer> m_vertexBuffer;
	std::shared_ptr<gl::Buffer> m_triangleBuffer;
	uint32_t m_numTrianglesPerLeaf;

	void LoadVertices( std::ifstream& _file, const FileDecl::NamedArray& _header );
	void LoadTriangles( std::ifstream& _file, const FileDecl::NamedArray& _header );
	void LoadMatRef( std::ifstream& _file, const FileDecl::NamedArray& _header );
};
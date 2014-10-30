#pragma once

#include "ei/3dtypes.hpp"
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
	template<typename T>
	struct TreeNode
	{
		uint32 firstChild;	///< Index of the first child, others are reached by the child's escape. 0 is the invalid index.
		uint32 escape;		///< Index of the next node when this completed. 0 is the invalid index.
		T boundingVolume;	///< The bounding volume is part of this node. Load different shader dependent of the type
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

	std::shared_ptr<gl::Buffer> GetVertexBuffer() const		{ return m_vertexBuffer; }
	std::shared_ptr<gl::Buffer> GetTriangleBuffer() const	{ return m_triangleBuffer; }
	std::shared_ptr<gl::Buffer> GetHierarchyBuffer() const	{ return m_hierarchyBuffer; }

	/// The bvh uses ε:: geometries. Which one can change with the files.
	ε::Types3D GetBoundingVolumeType() const	{ return m_bvType; }

	uint32 GetNumTrianglesPerLeaf() const		{ return m_numTrianglesPerLeaf; }
	uint32 GetNumInnerNodes() const				{ return m_numInnerNodes; }
private:
	std::shared_ptr<gl::Buffer> m_vertexBuffer;
	std::shared_ptr<gl::Buffer> m_triangleBuffer;
	std::shared_ptr<gl::Buffer> m_hierarchyBuffer;
	uint32_t m_numTrianglesPerLeaf;
	uint32_t m_numInnerNodes;
	ε::Types3D m_bvType;

	/// "sizeof" for different geometry
	size_t size(ε::Types3D _type);

	void LoadVertices( std::ifstream& _file, const FileDecl::NamedArray& _header );
	void LoadTriangles( std::ifstream& _file, const FileDecl::NamedArray& _header );
	void LoadMatRef( std::ifstream& _file, const FileDecl::NamedArray& _header );
	void LoadMatAssocialtion( std::ifstream& _file, const FileDecl::NamedArray& _header );
	void LoadHierarchy( std::ifstream& _file, const FileDecl::NamedArray& _header );
	void LoadBoundingVolumes( std::ifstream& _file, const FileDecl::NamedArray& _header );
};
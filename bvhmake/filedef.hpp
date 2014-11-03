#pragma once

#include "ei/matrix.hpp"
#include "../gpugi/utilities/assert.hpp"

namespace FileDecl
{
    /// \brief Kind of a chunk header.
    /// \details Files consist of named array data. It is possible that data
    ///     cross-references elements from the same or other arrays.
    struct NamedArray
    {
        char name[32];      ///< Names of arrays may have up to 31 characters
        uint32 numElements; ///< Number of elements in this array
        uint32 elementSize; ///< Size of on element in bytes
    };

    /// \brief Element type for geometry array (array: vertices).
    struct Vertex
    {
        ε::Vec3 position;
        ε::Vec3 normal;
        // TODO: tangente?
        ε::Vec2 texcoord;
    };

    /// \brief Element type for triangle lists (array: triangles).
    struct Triangle
    {
        uint32 vertices[3]; ///< Indices of the vertices (array: vertices)
    //    uint32 material;    ///< ID of the material (array: materials)
    };

    extern const Triangle INVALID_TRIANGLE;
    inline bool IsTriangleValid(const Triangle& _triangle)
	{
		Assert( (_triangle.vertices[0] != _triangle.vertices[1] && _triangle.vertices[0] != _triangle.vertices[2] && _triangle.vertices[2] != _triangle.vertices[1])
			|| (_triangle.vertices[0] == _triangle.vertices[1] && _triangle.vertices[0] == _triangle.vertices[2]), 
			"Invalid case!" );
		return _triangle.vertices[0] != _triangle.vertices[1];
	}

    /// \brief Element type for material lists (array: materials).
    struct Material
    {
        char material[32];  ///< Name of a material in an external material file
        uint32 firstVertex; ///< Index in (array: triangles) where the set of triangles with this material begins
        uint32 lastVertex;  ///< Index in (array: triangles) where the set of triangles with this material ends
    };

    /// \brief Element type for hierarchical nodes (array: hierarchy).
    /// \details The bounding volumes are stored in an equally-sized array
    ///     (bounding_aabox, bounding_sphere)
    ///
    ///     The most significant bit in the firstChild pointer is set, if a
    ///     node has a leaf child. If it is set the leaf is the only child
    struct Node
    {
        uint32 parent;       ///< This is useful on CPU side to navigate through the tree
        uint32 firstChild;   ///< The most significant bit determines if the child is a leaf (1)
        uint32 escape;       ///< Where to go next when this node was not hit?
    };

    /// \brief A leaf node references a list of N triangles (array: leafnodes).
    struct Leaf
    {
        static const uint NUM_PRIMITIVES = 8;
        Triangle triangles[NUM_PRIMITIVES];
        //uint32 numTriangles;
    };
}
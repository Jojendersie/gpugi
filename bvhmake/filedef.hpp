#pragma once

#include "ei/matrix.hpp"

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
        uint32 material;    ///< ID of the material (array: materials)
    };

    /// \brief Element type for material lists (array: materials).
    struct Material
    {
        char material[32];  ///< Name of a material in an external material file
        uint32 firstVertex; ///< Index in (array: triangles) where the set of triangles with this material begins
        uint32 lastVertex;  ///< Index in (array: triangles) where the set of triangles with this material ends
    };

    /// \brief Element type for hierarchical nodes (array: hierarchy).
    /// \details The bounding volumes are stored in an equally-sized array
    ///     (hierarchy_aabbox, hierarchy_bsphere)
    struct Node
    {
        uint32 parent;
        uint32 firstChild;
        uint32 next;
        uint32 flags;       ///< 1 if firstChild points to the leave array. If the child is a leaf there is only this one child
    };

    /// \brief A leaf node references a list of N triangles (array: leafnodes).
    struct Leaf
    {
        static const uint NUM_PRIMITIVES = 8;
        uint32 triangles[NUM_PRIMITIVES];
        uint32 numTriangles;
    };
}
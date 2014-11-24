﻿#pragma once

#include <unordered_map>
#include <functional>
#include <assimp/importer.hpp>
#include <ei/3dtypes.hpp>
#include <memory>
#include <jofilelib.hpp>

#include "filedef.hpp"
class BVHBuilder;

/// \brief The fit method decides which geometry should be used and how
///    this is computed.
/// \details An interface is necessary to be able to manage different types
///    automatically.
class FitMethod
{
public:
    enum class BVType
    {
        AABOX,
		SPHERE,
        AAELLIPSOID
    };

/*	/// \brief One time forward iterator
	class TriangleIterator
	{
	public:
		/// \brief Return the current and go to the next triangle.
		/// \return A triangle or nullptr if reached the end
		virtual const FileDecl::Triangle* next() const = 0;
	};*/

    /// \param [inout] The builder holds the memory which is accessed by the method.
    FitMethod(BVHBuilder* _manager) : m_manager(_manager) {}

    virtual ~FitMethod()    {}

    /// \brief Compute a new bounding volume from two child nodes.
    /// \param [in] _left The index of the left child and its BV respectively.
    /// \param [in] _right The index of the right child and its BV respectively.
    /// \param [in] _target The index of the new element.
	virtual void operator()(uint32 _left, uint32 _right, uint32 _target) const = 0;

    /// \brief Compute a new bounding volume from a leaf node.
    /// \param [in] _tringles List of valid triangles (e.g. from a leaf).
	/// \param [in] _num Number of triangls in the list.
    /// \param [in] _target The index of the new element.
    virtual void operator()(FileDecl::Triangle* _tringles, uint32 _num, uint32 _target) const = 0;

    /// \brief Get the geometry type generated by this functor (necessary
    ///    for memory management).
    virtual BVType Type() const = 0;

    /// \brief Get size for heuristics independent of the type.
    virtual float Surface(uint32 _index) const = 0;
    virtual float Volume(uint32 _index) const = 0;

    /// \brief Some builders require to know the minimal and maximal coordinates.
    /// \param [in] _dim x, y or z dimension (0, 1 or 2)
    virtual float GetMin(uint32 _index, int _dim) const = 0;
    virtual float GetMax(uint32 _index, int _dim) const = 0;
protected:
    BVHBuilder* m_manager;
};

class BuildMethod
{
public:
    /// \param [inout] The builder holds the memory which is accessed by the method.
    BuildMethod(BVHBuilder* _manager) : m_manager(_manager) {}

    virtual ~BuildMethod()  {}

    /// \brief Compute a whole tree.
    /// \return The index of the root element
    virtual uint32 operator()() const = 0;

    /// \brief Estimate the number of maximal tree nodes.
    /// \details It is important that the generator never takes
    ///     more nodes from BVHBuilder, but if the tree is reasonable small
    ///     try to allocate as few as possible.
    virtual void EstimateNodeCounts( uint32& _numInnerNodes, uint32& _numLeafNodes ) const = 0;

protected:
    BVHBuilder* m_manager;
};

/// \brief Central manager class for the import and export.
class BVHBuilder
{
public:
    /// \brief Registers all the different build implementations and sets the
    ///    default methods.
    BVHBuilder();

    ~BVHBuilder();

    /// \brief Get a comma separated list of all registered build types.
    std::string GetBuildMethods();

    /// \brief Get a comma separated list of all registered geometry types.
    std::string GetFitMethods();

    /// \brief Search for a named build method and set for the imports to come.
    /// \returns false if the build method is unknown
    bool SetBuildMethod( const char* _methodName );

    /// \brief Search for a named bounding geometry method and set for the
    ///    imports to come.
    /// \returns false if the type method is unknown
    bool SetGeometryType( const char* _geometryName );

    /// \brief Get the current fit method.
    /// \detail The build method is responsible to use this method and to
    ///     fill the array of bounding volumes with it.
    FitMethod* GetFitMethod()  { return m_fitMethod; }

	/// \brief Import all with assimp and preprocess.
    /// \returns Success or not.
    bool LoadSceneWithAssimp( const char* _file );

	/// \brief Load a material file.
	/// \details Without loading all materials are derived from the assimp
	///		import. Materials from the given file override the assimp materials
	///		if both have the same name.
	void LoadMaterials( const std::string& _materialFileName );

    /// \brief Write the vertex, triangle and material arrays to file
    void ExportGeometry( std::ofstream& _file );

    /// \brief Allocate space for the tree and the BVs and compute them.
    void BuildBVH();

    /// \brief Write the bounding volume hierarchy to file.
    void ExportBVH( std::ofstream& _file );

	/// \brief Create the "materialref", the "materialassociation" arrays
	///		and import new material entries for the json file.
	void ExportMaterials( std::ofstream& _file, const std::string& _materialFileName );

    /// \brief A tree node which should be used from any build method
    struct Node
    {
        uint32 left;            ///< Index of the left child in the node pool. The first bit denotes if the children is a leaf. In this case right is undefined
        uint32 right;           ///< Index of the right child in the node pool
    };

    uint32 GetTriangleCount() const { return m_triangleCount; }
    uint32 GetVertexCount() const { return m_vertexCount; }

    /// \brief Read/write access to bounding volumes
    template<typename T>
    T& GetBoundingVolume( uint32 _index )   { eiAssertWeak(_index < m_maxInnerNodeCount, "Out-of-Bounds!"); return static_cast<T*>(m_bvbuffer)[_index]; }

    /// \brief Read access to triangles.
    /// \details The triangle is constructed from index and vertex buffer on
    ///     call.
    ε::Triangle GetTriangle( uint32 _index ) const;
    ε::Triangle GetTriangle( FileDecl::Triangle _triangle ) const;

    /// \brief Get the index buffer for a triangle.
    FileDecl::Triangle GetTriangleIdx( uint32 _index ) const;

    /// \brief Allocate a new leaf from the pool.
    /// \returns Index of the new leaf.
    uint32 GetNewLeaf();

    /// \brief Get write access to the leaf memory.
    FileDecl::Leaf& GetLeaf( uint32 _index ) { return m_leaves[_index]; }

    /// \brief Allocate a new inner node from the pool.
    uint32 GetNewNode();

    /// \brief Get write access to the leaf memory.
    Node& GetNode( uint32 _index ) { return m_nodes[_index]; }

private:
	Jo::Files::MetaFileWrapper m_materials;
    BuildMethod* m_buildMethod;
    FitMethod* m_fitMethod;
    std::unordered_map<std::string, BuildMethod*> m_buildMethods;
    std::unordered_map<std::string, FitMethod*> m_fitMethods;
    Assimp::Importer m_importer;	///< Importer which is cleaning up everything automatically
    const struct aiScene* m_scene;	///< The Assimp scene from the loaded file.
    std::unique_ptr<ε::Vec3[]> m_positions;
    std::unique_ptr<uint32[]> m_triangles;
    uint32 m_triangleCount;         ///< Point to the end of m_triangles during filling
    uint32 m_vertexCount;           ///< Point to the end of m_positions during filling

    // Memory during build
    void* m_bvbuffer;               ///< Buffer containing space for m_maxInnerNodeCount bounding volumes
    Node* m_nodes;                  ///< Buffer for all m_maxInnerNodeCount tree nodes.
    FileDecl::Leaf* m_leaves;       ///< Buffer for all tree leaves.
    uint32 m_innerNodeCount, m_maxInnerNodeCount;
    uint32 m_leafNodeCount, m_maxLeafNodeCount;

    /// \brief Prepare headers for geometry export by counting elements
    /// \param [out] _numVertices Counter for the vertices must be 0 before call.
    /// \param [out] _numFaces Counter for the vertices must be 0 before call.
    void CountGeometry( const struct aiNode* _node, uint& _numVertices, uint& _numFaces );
    /// \brief Recursive export function
    void ExportVertices( std::ofstream& _file,
        const struct aiNode* _node,
        const ε::Mat4x4& _transformation );

	/// \brief Converts Node(s) to FileDecl::Node(s)
	void RecursiveWriteHierarchy( std::ofstream& _file, uint32 _this, uint32 _parent, uint32 _escape );
};
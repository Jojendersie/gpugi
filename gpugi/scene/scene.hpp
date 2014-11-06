﻿#pragma once

#include "ei/3dtypes.hpp"
#include "../glhelper/buffer.hpp"
#include "../glhelper/texture2D.hpp"
#include "../glhelper/samplerobject.hpp"
#include "../../bvhmake/filedef.hpp"
#include <jofilelib.hpp>

#include <string>
#include <memory>
#include <unordered_map>

/// Scene manager - loads data from a file and provides it for the CPU/GPU.
class Scene
{
public:
    /// Load a scene.
    Scene( const std::string& _file );

    /// Unload all the scene data and GPU resources
    ~Scene();

	/// Inner node struct on GPU side
	/// Specialized for optimal GPU alignment.
	template<typename T>
	struct TreeNode {	};
	template<>
	struct TreeNode<ei::Box>
	{
		ei::Vec3 min;
		uint32 firstChild;	///< Index of the first child, others are reached by the child's escape. 0 is the invalid index.

		ei::Vec3 max;
		uint32 escape;		///< Index of the next node when this completed. 0 is the invalid index.
	};
	template<>
	struct TreeNode<ei::Sphere>
	{
		ei::Vec3 center;
		float radius;
		
		uint32 firstChild;	///< Index of the first child, others are reached by the child's escape. 0 is the invalid index.
		uint32 escape;		///< Index of the next node when this completed. 0 is the invalid index.
		
		uint32 padding[2]; ///< Seems to be necessary
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

	/// GPU information for a material with several bindless textures and
	/// (material-)global constants.
	struct Material
	{
		uint64 diffuseTexHandle;
		uint64 opacityTexHandle;
		uint64 reflectivenessTexHandle;
		ε::Vec2 emissivityRG;			///< Split emissive for padding reasons
		ε::Vec3 fresnel0;				///< Fist precomputed coefficient for Fresnel approximation (rgb)
		float refractionIndexAvg;
		ε::Vec3 fresnel1;				///< Second precomputed coefficient for Fresnel approximation (rgb)
		float emissivityB;				///< Split emissive for padding reasons
	};

	/// Light source triangles.
	/// \details It is assumed that there are not that much emissive triangles
	///		and therefore vertices are stored directly instead of using index
	///		buffers.
	struct LightTriangle 
	{
		ε::Vec3 position[3];
		ε::Vec3 normal[3];
		ε::Vec3 luminance;
	};

	std::shared_ptr<gl::Buffer> GetVertexBuffer() const		{ return m_vertexBuffer; }
	std::shared_ptr<gl::Buffer> GetTriangleBuffer() const	{ return m_triangleBuffer; }
	std::shared_ptr<gl::Buffer> GetHierarchyBuffer() const	{ return m_hierarchyBuffer; }

	/// The bvh uses ε:: geometries. Which one can change with the files.
	ε::Types3D GetBoundingVolumeType() const	{ return m_bvType; }

	uint32 GetNumTrianglesPerLeaf() const		{ return m_numTrianglesPerLeaf; }
	uint32 GetNumInnerNodes() const				{ return m_numInnerNodes; }
	uint32 GetNumLightTriangles() const			{ return static_cast<uint32>(m_lightTriangles.size()); }
	const LightTriangle* GetLightTriangles() const	{ return m_lightTriangles.data(); }
	// Normalized (to [0,1]) summed area for all light triangles
	const float* GetLightSummedArea() const			{ return m_lightSummedArea.data(); }
private:
	std::shared_ptr<gl::Buffer> m_vertexBuffer;
	std::shared_ptr<gl::Buffer> m_triangleBuffer;
	std::shared_ptr<gl::Buffer> m_hierarchyBuffer;
	std::vector<LightTriangle> m_lightTriangles;
	std::vector<float> m_lightSummedArea;
	std::vector<Material> m_materials;
	uint32_t m_numTrianglesPerLeaf;
	uint32_t m_numInnerNodes;
	ε::Types3D m_bvType;
	std::unordered_map<std::string, std::unique_ptr<gl::Texture2D>> m_textures;
	const gl::SamplerObject& m_samplerLinearNoMipMap;

	/// "sizeof" for different geometry
	size_t size(ε::Types3D _type);

	void LoadVertices( std::ifstream& _file, const FileDecl::NamedArray& _header );
	void LoadTriangles( std::ifstream& _file, const FileDecl::NamedArray& _header );
	void LoadMatRef( std::ifstream& _file, const Jo::Files::MetaFileWrapper::Node& _materials, const FileDecl::NamedArray& _header );
	void LoadHierarchy( std::ifstream& _file, const FileDecl::NamedArray& _header );
	void LoadBoundingVolumes( std::ifstream& _file, const FileDecl::NamedArray& _header );
	/// Analyzes the data and searches the emissive triangles. Requires the other
	/// methods to be executed before.
	void LoadLightSources( std::ifstream& _file );

	/// Load/create textures and read the other texture parameters.
	void LoadMaterial( const Jo::Files::MetaFileWrapper::Node& _material );
	/// Load texture from file
	uint64 GetBindlessHandle( const std::string& _name );
	/// Create RGB8 texture with single data value
	uint64 GetBindlessHandle( const ε::Vec3& _data );
	/// Create RGBA32F texture with single data value
	uint64 GetBindlessHandle( const ε::Vec4& _data );
};
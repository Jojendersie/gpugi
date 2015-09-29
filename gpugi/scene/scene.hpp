﻿#pragma once

#include <ei/3dtypes.hpp>
#include <glhelper/buffer.hpp>
#include <glhelper/texture2D.hpp>
#include <glhelper/samplerobject.hpp>
#include <jofilelib.hpp>

#include "../../bvhmake/filedef.hpp"

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

	/// GPU representation of non-position vertex part (not as frequently needed)
	struct VertexInfo
	{
        ε::Vec2 normalAngles; // atan2(normal.y, normal.x), normal.z
        ε::Vec2 texcoord;
	};

	/// GPU information for a material with several bindless textures and
	/// (material-)global constants.
	struct Material
	{
		uint64 diffuseTexHandle;
		uint64 opacityTexHandle;
		uint64 reflectivenessTexHandle;
		uint64 emissivityTexHandle;
		ε::Vec3 fresnel0;				///< Fist precomputed coefficient for Fresnel approximation (rgb)
		float refractionIndexAvg;
		ε::Vec3 fresnel1;				///< Second precomputed coefficient for Fresnel approximation (rgb)
		float padding;
	};

	/// Light source triangles.
	/// \details It is assumed that there are not that much emissive triangles
	///		and therefore vertices are stored directly instead of using index
	///		buffers.
	struct LightTriangle 
	{
		ε::Triangle triangle;
		//ε::Vec2 texcoord[3];
		ε::Vec3 luminance;
	};

	struct PointLight
	{
		ε::Vec3 position;
		ε::Vec3 intensity;
	};

	std::shared_ptr<gl::Buffer> GetVertexPositionBuffer() const	{ return m_vertexPositionBuffer; }
	std::shared_ptr<gl::Buffer> GetVertexInfoBuffer() const		{ return m_vertexInfoBuffer; }
	std::shared_ptr<gl::Buffer> GetTriangleBuffer() const		{ return m_triangleBuffer; }
	std::shared_ptr<gl::Buffer> GetHierarchyBuffer() const		{ return m_hierarchyBuffer; }
	/// The parent buffer supplements the hierachy buffer.
	/// It contains a parent index for each node.
	std::shared_ptr<gl::Buffer> GetParentBuffer() const			{ return m_parentBuffer; }
	/// A CPU sided version of the parent buffer.
	const std::vector<uint32>& GetParentBufferRAM() const		{ return m_parentBufferRAM; }

	const std::vector<Material>& GetMaterials() const			{ return m_materials; }
	const std::vector<PointLight>& GetPointLights() const		{ return m_pointLights; }

	/// The bvh uses ε:: geometries. Which one can change with the files.
	ε::Types3D GetBoundingVolumeType() const	{ return m_bvType; }

	uint32 GetNumTrianglesPerLeaf() const		{ return m_numTrianglesPerLeaf; }
	uint32 GetNumTriangles() const				{ return static_cast<uint32>(m_triangleBuffer->GetSize() / sizeof(Triangle)); }
	uint32 GetNumInnerNodes() const				{ return m_numInnerNodes; }
	uint32 GetNumMaterials() const				{ return static_cast<uint32>(m_materials.size()); }
	uint32 GetNumVertices() const				{ return static_cast<uint32>(m_vertexPositionBuffer->GetSize() / sizeof(ei::Vec3)); }
	uint32 GetNumLightTriangles() const			{ return static_cast<uint32>(m_lightTriangles.size()); }
	const LightTriangle* GetLightTriangles() const	{ return m_lightTriangles.data(); }
	// Normalized (to [0,1]) summed area for all light triangles
	const float* GetLightSummedAreaTable() const			{ return m_lightSummedArea.data(); }
	// Normalized (to [0,1]) summed flux for all point light sources
	const float* GetPointLightSummedFluxTable() const		{ return m_pointLightSummedFlux.data(); }
	float GetLightAreaSum() const				{ return m_lightAreaSum; }
	float GetTotalAreaLightFlux() const			{ return m_totalAreaLightFlux; }
	float GetTotalPointLightFlux() const		{ return m_totalPointLightFlux; }
	const ε::Box& GetBoundingBox() const		{ return m_boundingBox; }

	// Add a new point light source to the scene
	void AddPointLight(const PointLight& _light) { m_pointLights.push_back(_light); ComputePointLightTable(); }
	// Returns false if no light with the index excists
	bool SetPointLight(size_t _index, const PointLight& _light) { if(_index >= m_pointLights.size()) return false; m_pointLights[_index] = _light; return true; ComputePointLightTable(); }
//	bool RemovePointLight(size_t _index) { if(_index >= m_pointLights.size()) return false; m_pointLights[_index] = m_pointLights.back(); m_pointLights.pop_back(); return true; }

private:
	std::shared_ptr<gl::Buffer> m_vertexPositionBuffer;
	std::shared_ptr<gl::Buffer> m_vertexInfoBuffer;
	std::shared_ptr<gl::Buffer> m_triangleBuffer;
	std::shared_ptr<gl::Buffer> m_hierarchyBuffer;
	std::shared_ptr<gl::Buffer> m_parentBuffer;
	std::vector<uint32> m_parentBufferRAM;

	ε::Box m_boundingBox;
	std::vector<LightTriangle> m_lightTriangles;
	std::vector<PointLight> m_pointLights;
	std::vector<float> m_lightSummedArea;
	std::vector<float> m_pointLightSummedFlux;
	std::vector<Material> m_materials;
	std::vector<ε::Vec3> m_emissivity;	///< Additional material info. The emissivity is used to sample virtual lights
	uint64 m_noFluxEmissiveTexture;		///< Dummy texture for all unlit surfaces
	uint32 m_numTrianglesPerLeaf;
	uint32 m_numInnerNodes;
	float m_lightAreaSum;
	float m_totalAreaLightFlux, m_totalPointLightFlux;
	ε::Types3D m_bvType;
	std::unordered_map<std::string, std::unique_ptr<gl::Texture2D>> m_textures;
	const gl::SamplerObject& m_samplerLinearNoMipMap;

	std::string m_sourceDirectory;

	/// "sizeof" for different geometry
	size_t size(ε::Types3D _type);

	std::unique_ptr<FileDecl::Vertex[]> LoadVertices(std::ifstream& _file, const FileDecl::NamedArray& _header);
	std::unique_ptr<Triangle[]> LoadTriangles( std::ifstream& _file, const FileDecl::NamedArray& _header );
	void LoadMatRef( std::ifstream& _file, const Jo::Files::MetaFileWrapper::Node& _materials, const FileDecl::NamedArray& _header );
	void LoadHierarchy( std::ifstream& _file, const FileDecl::NamedArray& _header );
	void LoadBoundingVolumes( std::ifstream& _file, const FileDecl::NamedArray& _header );
	/// Analyzes the data and searches the emissive triangles. Requires the other
	/// methods to be executed before.
	/// \param _triangles Temporary double buffer of all triangles.
	/// \param _vertices Temporary double buffer of all vertices.
	void LoadLightSources( std::unique_ptr<Triangle[]> _triangles, std::unique_ptr<FileDecl::Vertex[]> _vertices );
	/// Analyzes the point light list to precompute values of importance sampling.
	void ComputePointLightTable();

	/// Check references between indices, vertices and materials
	void SanityCheck(Triangle* _triangles);

	/// Load/create textures and read the other texture parameters.
	void LoadMaterial( const Jo::Files::MetaFileWrapper::Node& _material );
	/// Load texture from file and makes it resident
	uint64 GetBindlessHandle( const std::string& _name );
	/// Create RGB8 texture with single data value and makes it resident
	uint64 GetBindlessHandle( const ε::Vec3& _data );
	/// Create RGBA32F texture with single data value and makes it resident
	uint64 GetBindlessHandle( const ε::Vec4& _data );
};

// IDEA for out of core loading support:
// once a header is found a cached reader is initialized to that point.
// Both stream and random access should be done via cached reader.
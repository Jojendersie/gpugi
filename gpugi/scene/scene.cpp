#include "scene.hpp"
#include <glhelper/gl.hpp>
#include <glhelper/samplerobject.hpp>
#include <glhelper/texture2d.hpp>
#include "../utilities/logger.hpp"
#include "../utilities/assert.hpp"
#include "../utilities/flagoperators.hpp"
#include "../dependencies/glhelper/glhelper/utils/pathutils.hpp"
#include <ei/3dtypes.hpp>

#include <fstream>

using namespace bim;

Scene::Scene( const std::string& _file, ε::Types3D _bvhType ) :
	m_samplerLinearNoMipMap( gl::SamplerObject::GetSamplerObject(gl::SamplerObject::Desc(
		gl::SamplerObject::Filter::NEAREST,
		gl::SamplerObject::Filter::LINEAR,
		gl::SamplerObject::Filter::NEAREST,
		gl::SamplerObject::Border::REPEAT
	)) ),
	m_totalPointLightFlux( 0.0f ),
	m_totalAreaLightFlux( 0.0f ),
	m_lightAreaSum( 0.0f )
{
	m_sourceDirectory = PathUtils::GetDirectory(_file);
	m_bvhType = _bvhType;

	// Load materials from dedicated material file
	if( !Jo::Files::Utils::Exists(_file) )
		LOG_ERROR("No scene description file '" + _file + "' found.");

	Property::Val bvhProp;
	switch(_bvhType) {
		case ε::Types3D::BOX: bvhProp = Property::AABOX_BVH; break;
		case ε::Types3D::OBOX: bvhProp = Property::OBOX_BVH; break;
	}
	if(!m_model.load(_file.c_str(),
		Property::Val(Property::NORMAL | Property::TEXCOORD0 | bvhProp | Property::HIERARCHY | Property::TRIANGLE_MAT),
		Property::NDF_SGGX))
	{
		LOG_ERROR("Failed to load scene " + _file);
		return;
	}
	m_model.makeChunkResident(ε::IVec3(0));
	m_sceneChunk = m_model.getChunk(ε::IVec3(0));

	// The scene is loaded now, but must be mapped to GPU
	UploadGeometry();
	UploadHierarchy(_bvhType);
	for(uint i = 0; i < m_model.getNumUsedMaterials(); ++i)
		LoadMaterial(*m_model.getMaterial(i));

	LoadLightSources();
}

Scene::~Scene()
{
	// Make all textures non resident
	for( auto& it : m_textures )
	{
		uint64 handle = GL_RET_CALL(glGetTextureSamplerHandleARB, it.second->GetInternHandle(), m_samplerLinearNoMipMap.GetInternHandle());
	}
}

const char* Scene::GetBvhTypeDefineString() const
{
	switch(m_bvhType) {
		case ε::Types3D::BOX: return "#define AABOX_BVH\n";
		case ε::Types3D::OBOX: return "#define OBOX_BVH\n";
	}
	return nullptr;
}

void Scene::UploadGeometry()
{
	std::vector<VertexInfo> infoData;
	infoData.resize(m_sceneChunk->getNumVertices());
	for(uint v = 0; v < m_sceneChunk->getNumVertices(); ++v)
	{
		infoData[v].normalAngles.x = atan2(m_sceneChunk->getNormals()[v].y, m_sceneChunk->getNormals()[v].x);
		infoData[v].normalAngles.y = m_sceneChunk->getNormals()[v].z;
		infoData[v].texcoord = m_sceneChunk->getTexCoords0()[v];
	}

	// Allocate and upload directly (immutable resources are faster, but need the data on setup)
	m_vertexPositionBuffer = std::make_shared<gl::Buffer>(static_cast<std::uint32_t>(sizeof(ei::Vec3) * m_sceneChunk->getNumVertices()), gl::Buffer::IMMUTABLE, m_sceneChunk->getPositions());
	m_vertexInfoBuffer = std::make_shared<gl::Buffer>(static_cast<std::uint32_t>(sizeof(VertexInfo) * m_sceneChunk->getNumVertices()), gl::Buffer::IMMUTABLE, infoData.data());
	m_triangleBuffer = std::make_shared<gl::Buffer>( uint32(m_model.getNumTrianglesPerLeaf() * sizeof(ei::UVec4) * m_sceneChunk->getNumLeafNodes()), gl::Buffer::IMMUTABLE, m_sceneChunk->getLeafNodes() );
}

void Scene::UploadHierarchy(ε::Types3D _bvhType)
{
	// Prepare data for upload
	std::vector<char> hierarchy;
	if(_bvhType == ε::Types3D::BOX)
	{
		hierarchy.reserve(sizeof(TreeNode<ε::Box>) * m_sceneChunk->getNumNodes());
		TreeNode<ε::Box>* hierarchyData = reinterpret_cast<TreeNode<ε::Box>*>(hierarchy.data());
		for(uint i = 0; i < m_sceneChunk->getNumNodes(); ++i)
		{
			hierarchyData[i].min = m_sceneChunk->getHierarchyAABoxes()[i].min;
			hierarchyData[i].max = m_sceneChunk->getHierarchyAABoxes()[i].max;
			hierarchyData[i].escape = m_sceneChunk->getHierarchy()[i].escape;
			hierarchyData[i].firstChild = m_sceneChunk->getHierarchy()[i].firstChild;
		}
	} else if(_bvhType == ε::Types3D::OBOX)
	{
		hierarchy.reserve(sizeof(TreeNode<ε::OBox>) * m_sceneChunk->getNumNodes());
		TreeNode<ε::OBox>* hierarchyData = reinterpret_cast<TreeNode<ε::OBox>*>(hierarchy.data());
		for(uint i = 0; i < m_sceneChunk->getNumNodes(); ++i)
		{
			hierarchyData[i].center = m_sceneChunk->getHierarchyOBoxes()[i].center;
			hierarchyData[i].sidesHalf = m_sceneChunk->getHierarchyOBoxes()[i].halfSides;
			hierarchyData[i].rotationInv = conjugate(m_sceneChunk->getHierarchyOBoxes()[i].orientation);
			hierarchyData[i].escape = m_sceneChunk->getHierarchy()[i].escape;
			hierarchyData[i].firstChild = m_sceneChunk->getHierarchy()[i].firstChild;
		}
	}
	// Allocate and upload
	if(_bvhType == ε::Types3D::BOX)
		m_hierarchyBuffer = std::make_shared<gl::Buffer>(uint32(sizeof(TreeNode<ε::Box>) * m_sceneChunk->getNumNodes()), gl::Buffer::IMMUTABLE, hierarchy.data());
	else if(_bvhType == ε::Types3D::OBOX)
		m_hierarchyBuffer = std::make_shared<gl::Buffer>(uint32(sizeof(TreeNode<ε::OBox>) * m_sceneChunk->getNumNodes()), gl::Buffer::IMMUTABLE, hierarchy.data());
	m_parentBuffer = std::make_shared<gl::Buffer>(uint32(4 * m_sceneChunk->getNumNodes()), gl::Buffer::IMMUTABLE, m_sceneChunk->getHierarchyParents());

	// Upload SGGX NDFs only if available
	if(m_sceneChunk->getNodeNDFs())
		m_sggxBuffer = std::make_shared<gl::Buffer>(sizeof(bim::SGGX) * m_sceneChunk->getNumNodes(), gl::Buffer::IMMUTABLE, m_sceneChunk->getNodeNDFs());
}

void Scene::LoadMaterial( const bim::Material& _material )
{
	Material mat;
	ε::Vec3 emissivity(0.0f); // Additional material info for uniform area lights
	m_noFluxEmissiveTexture = GetBindlessHandle(ε::Vec3(0.0f), false);
	// "String pool"
	static const std::string s_emissivity("emissivity");
	static const std::string s_refrN("refractionIndexN");
	static const std::string s_refrK("refractionIndexK");
	static const std::string s_fresnel0("specularColor");
	static const std::string s_diffuse("albedo");
	static const std::string s_opacity("opacity");
	static const std::string s_specular("reflectiveness");
	static const std::string s_roughness("roughness");
	try {
		// Refraction parameters for Fresnel
	/*	ε::Vec3 n = _material.get(s_refrN, ε::Vec3(1.0f));
		ε::Vec3 k = _material.get(s_refrK, ε::Vec3(0.0f));
		ε::Vec3 den = 1.0f / (sq(n+1.0f) + sq(k));
		mat.refractionIndexAvg = avg(n);
		mat.fresnel0 = (sq(n-1.0f) + sq(k)) * den;
		mat.fresnel1 = 4.0f * n * den;*/
		mat.refractionIndexAvg = 1.3f;
		mat.fresnel0 = _material.get(s_fresnel0, ei::Vec3(0.03f));
		mat.fresnel1 = 1.0f - mat.fresnel0;
		// Specular exponent parameter
		float roughness = _material.get(s_roughness, 1.0f);
		float exponent = 1.0f / (roughness * roughness + 1e-20f);
		// Load or create diffuse texture
		if(_material.getTexture(s_diffuse))
			mat.diffuseTexHandle = GetBindlessHandle(*_material.getTexture(s_diffuse));
		else mat.diffuseTexHandle = GetBindlessHandle(_material.get(s_diffuse, ε::Vec4(0.5f)).subcol<0,3>(), false);
		// Load or create opacity texture
		if(_material.getTexture(s_opacity))
			mat.opacityTexHandle = GetBindlessHandle(*_material.getTexture(s_opacity));
		else mat.opacityTexHandle = GetBindlessHandle(_material.get(s_opacity, ε::Vec4(1.0f)).subcol<0,3>(), false);
		// Load or create specular texture
		if(_material.getTexture(s_specular))
			mat.reflectivenessTexHandle = GetBindlessHandle(*_material.getTexture(s_specular));
		else mat.reflectivenessTexHandle = GetBindlessHandle(_material.get(s_specular, ε::Vec4(1.0f, 1.0f, 1.0, exponent)), true);
		// Load or create emissive texture
		if(_material.getTexture(s_emissivity))
			mat.emissivityTexHandle = GetBindlessHandle(*_material.getTexture(s_emissivity));
		else{
			emissivity = _material.get(s_emissivity, ε::Vec3(0.0f));
			mat.emissivityTexHandle = GetBindlessHandle(emissivity, true);
		}
	} catch(const std::string& _msg ) {
		LOG_ERROR("Failed to load the material. Exception: " + _msg);
	} catch(...) {
		LOG_ERROR("Failed to load the material. Material file or textures corrupted (unknown exception).");
	}
	m_materials.push_back( mat );
	m_emissivity.push_back( emissivity );
}

uint64 Scene::GetBindlessHandle( const std::string& _name )
{
	uint64 handle;
	auto it = m_textures.find(_name);
	if( it == m_textures.end() )
	{
		it = m_textures.insert( std::pair<std::string, std::unique_ptr<gl::Texture2D>>(
			_name, gl::Texture2D::LoadFromFile(PathUtils::AppendPath(m_sourceDirectory, _name), false, true))).first;
		handle = GL_RET_CALL(glGetTextureSamplerHandleARB, it->second->GetInternHandle(), m_samplerLinearNoMipMap.GetInternHandle());
		// Make permanently resident
		GL_CALL(glMakeTextureHandleResidentARB, handle);
	} else
		handle = GL_RET_CALL(glGetTextureSamplerHandleARB, it->second->GetInternHandle(), m_samplerLinearNoMipMap.GetInternHandle());
	
	return handle;
}

uint64 Scene::GetBindlessHandle( const ε::Vec3& _data, bool _hdr )
{
	uint64 handle;
	std::string genName = "gen" + std::to_string(_data.x) + '_' + std::to_string(_data.y) + '_' + std::to_string(_data.z) + (_hdr?"_1":"_0");
	auto it = m_textures.find(genName);
	if( it == m_textures.end() )
	{
		gl::TextureFormat format = _hdr ? gl::TextureFormat::RGB32F : gl::TextureFormat::RGB8;
		it = m_textures.insert( std::pair<std::string, std::unique_ptr<gl::Texture2D>>(
			genName, std::unique_ptr<gl::Texture2D>(new gl::Texture2D(1, 1, format, &_data, gl::TextureSetDataFormat::RGB, gl::TextureSetDataType::FLOAT))) ).first;
		handle = GL_RET_CALL(glGetTextureSamplerHandleARB, it->second->GetInternHandle(), m_samplerLinearNoMipMap.GetInternHandle());
		// Make permanently resident
		GL_CALL(glMakeTextureHandleResidentARB, handle);
	} else
		handle = GL_RET_CALL(glGetTextureSamplerHandleARB, it->second->GetInternHandle(), m_samplerLinearNoMipMap.GetInternHandle());
	
	return handle;
}

uint64 Scene::GetBindlessHandle( const ε::Vec4& _data, bool _hdr )
{
	uint64 handle;
	std::string genName = "gen" + std::to_string(_data.x) + '_' + std::to_string(_data.y) + '_' + std::to_string(_data.z) + '_' + std::to_string(_data.w) + (_hdr?"_1":"_0");
	auto it = m_textures.find(genName);
	if( it == m_textures.end() )
	{
		gl::TextureFormat format = _hdr ? gl::TextureFormat::RGBA32F : gl::TextureFormat::RGBA8;
		it = m_textures.insert( std::pair<std::string, std::unique_ptr<gl::Texture2D>>(
			genName, std::unique_ptr<gl::Texture2D>(new gl::Texture2D(1, 1, format, &_data, gl::TextureSetDataFormat::RGBA, gl::TextureSetDataType::FLOAT))) ).first;
		handle = GL_RET_CALL(glGetTextureSamplerHandleARB, it->second->GetInternHandle(), m_samplerLinearNoMipMap.GetInternHandle());
		// Make permanently resident
		GL_CALL(glMakeTextureHandleResidentARB, handle);
	} else
		handle = GL_RET_CALL(glGetTextureSamplerHandleARB, it->second->GetInternHandle(), m_samplerLinearNoMipMap.GetInternHandle());
	
	return handle;
}

void Scene::LoadLightSources()
{
	m_totalAreaLightFlux = 0.0f;
	float sum = 0.0f;
	// Read the buffers with SubDataGets (still faster than a second file read pass)
	for(uint32_t i = 0; i < m_sceneChunk->getNumTriangles(); ++i)
	{
		const ε::UVec3& tri = m_sceneChunk->getTriangles()[i];
		// Is this a valid light source triangle?
		if( tri[0] != tri[1]
			&& (m_emissivity[m_sceneChunk->getTriangleMaterials()[i]] != ε::Vec3(0.0f)) )
		{
			LightTriangle lightSource;
			lightSource.luminance = m_emissivity[m_sceneChunk->getTriangleMaterials()[i]];
			//lightSource.emissivityTexHandle = m_materials[_triangles[i].material].emissivityTexHandle;
			// Get the 3 vertices from vertex buffer
			//for(int j = 0; j < 3; ++j)
			//	lightSource.texcoord[j] = _vertices[_triangles[i].vertices[j]].texcoord;
			lightSource.triangle.v0 = m_sceneChunk->getPositions()[tri[0]];
			lightSource.triangle.v1 = m_sceneChunk->getPositions()[tri[1]];
			lightSource.triangle.v2 = m_sceneChunk->getPositions()[tri[2]];
			m_lightTriangles.push_back(lightSource);

			// Flux
			float area = ε::surface(lightSource.triangle);
			m_totalAreaLightFlux += dot(ε::Vec3(0.2126f, 0.7152f, 0.0722f), lightSource.luminance) * area * ε::π; // π is the integral over all solid angles of the cosine lobe
			// Assume average luminance of 0.5
			//m_totalAreaLightFlux += 0.5f * area * ε::π;

			// Compute the area
			sum += area;
			m_lightSummedArea.push_back(sum);
		}
	}

	// Normalize the sum
	if(!m_lightSummedArea.empty())
	{
		m_lightAreaSum = m_lightSummedArea.back();
		for (size_t i = 0; i < m_lightSummedArea.size(); ++i)
			m_lightSummedArea[i] /= m_lightAreaSum;
	}

	// Normalize material emissivities
	/*for (Material& mat : m_materials)
	{
		mat.emissivityRG /= m_lightAreaSum;
		mat.emissivityB /= m_lightAreaSum;
	}*/
}

void Scene::ComputePointLightTable()
{
	m_totalPointLightFlux = 0.0f;
	m_pointLightSummedFlux.clear();
	for(auto& light : m_pointLights)
	{
		float flux = dot(ε::Vec3(0.2126f, 0.7152f, 0.0722f), light.intensity) * 4 * ε::π;
		m_totalPointLightFlux += log(1.0f + flux);
		m_pointLightSummedFlux.push_back( m_totalPointLightFlux );
	}
	for(size_t i = 0; i < m_pointLightSummedFlux.size(); ++i)
		m_pointLightSummedFlux[i] /= m_totalPointLightFlux;
}

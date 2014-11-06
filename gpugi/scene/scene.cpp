#include "scene.hpp"
#include "../glhelper/gl.hpp"
#include "../glhelper/samplerobject.hpp"
#include "../glhelper/texture2d.hpp"
#include "../utilities/logger.hpp"
#include "../utilities/assert.hpp"

#include <fstream>

Scene::Scene( const std::string& _file ) :
	m_numTrianglesPerLeaf(0),
	m_bvType( ε::Types3D::NUM_TYPES ),	// invalid type so far
	m_samplerLinearNoMipMap( gl::SamplerObject::GetSamplerObject(gl::SamplerObject::Desc(
		gl::SamplerObject::Filter::NEAREST,
		gl::SamplerObject::Filter::LINEAR,
		gl::SamplerObject::Filter::NEAREST,
		gl::SamplerObject::Border::REPEAT
	)) )
{
	// Load materials from dedicated material file
	std::string matFileName = _file.substr(0, _file.find_last_of('.')) + ".json";
	Jo::Files::MetaFileWrapper materialFile;
	if( Jo::Files::Utils::Exists(matFileName) )
		materialFile.Read( Jo::Files::HDDFile(matFileName) );
	else
		LOG_ERROR("No material file '" + matFileName + "' found.");

	std::ifstream file( _file, std::ios_base::binary );
	if( file.fail() )
	{
		LOG_ERROR( "Cannot open scene file \"" + _file + "\"!" );
		return;
	}

	// First find the bounding volume header. Otherwise the hierarchy
	// cannot be build correctly.
	while( !file.eof() )
	{
		FileDecl::NamedArray header;
		file.read( reinterpret_cast<char*>(&header), sizeof(header) );
		if(strcmp(header.name, "bounding_aabox") == 0) { m_bvType = ε::Types3D::BOX; m_numInnerNodes = header.numElements; break; }
		if(strcmp(header.name, "bounding_sphere") == 0) { m_bvType = ε::Types3D::SPHERE; m_numInnerNodes = header.numElements; break; }
		file.seekg( header.elementSize * header.numElements, std::ios_base::cur );
	}

	if( m_bvType == ε::Types3D::NUM_TYPES )
	{
		LOG_ERROR("Cannot load file. It does not contain a bounding volume array!");
		return;
	}
	if( m_bvType == ε::Types3D::BOX )
		m_hierarchyBuffer = std::make_shared<gl::Buffer>(uint32(sizeof(TreeNode<ε::Box>) * m_numInnerNodes), gl::Buffer::Usage::MAP_WRITE);
	else if( m_bvType == ε::Types3D::SPHERE )
		m_hierarchyBuffer = std::make_shared<gl::Buffer>(uint32(sizeof(TreeNode<ε::Sphere>) * m_numInnerNodes), gl::Buffer::Usage::MAP_WRITE);

	// Read one data array after another. The order is undefined.
	file.seekg( 0 );
	FileDecl::NamedArray header;
	while( file.read( reinterpret_cast<char*>(&header), sizeof(header) ) )
	{
		if(strcmp(header.name, "vertices") == 0) LoadVertices(file, header);
		else if(strcmp(header.name, "triangles") == 0) LoadTriangles(file, header);
		else if(strcmp(header.name, "materialref") == 0) LoadMatRef(file, materialFile.RootNode, header);
		else if(strcmp(header.name, "hierarchy") == 0) LoadHierarchy(file, header);
		else if(strcmp(header.name, "bounding_aabox") == 0) LoadBoundingVolumes(file, header);
		else if(strcmp(header.name, "bounding_sphere") == 0) LoadBoundingVolumes(file, header);
		Assert( !file.fail(), "Failed to load the last block correctly." );
	}

	if( m_numTrianglesPerLeaf == 0 )
		LOG_ERROR("Did not find a \"triangles\" buffer in" + _file + "!");
	if( m_vertexBuffer == nullptr )
		LOG_ERROR("Did not find a \"vertices\" buffer in" + _file + "!");
	if( m_hierarchyBuffer == nullptr )
		LOG_ERROR("Did not find a \"hierarchy\" buffer in" + _file + "!");	
}

Scene::~Scene()
{
}

size_t Scene::size(ε::Types3D _type)
{
	static size_t s_sizes[] = {sizeof(ε::Sphere), 0/*sizeof(ε::Plane)*/, sizeof(ε::Box),
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0/*sizeof(ε::OBox), sizeof(ε::Disc), sizeof(ε::Triangle),
		sizeof(ε::Thetrahedron), sizeof(ε::Ray), sizeof(ε::Line), sizeof(ε::Frustum),
		sizeof(ε::PyramidFrustum), sizeof(ε::Ellipsoid), sizeof(ε::OEllipsoid),
		sizeof(ε::Capsule)*/};
	return s_sizes[int(_type)];
}

void Scene::LoadVertices( std::ifstream& _file, const FileDecl::NamedArray& _header )
{
	Assert( _header.elementSize == sizeof(Vertex), "Format seems to contain other vertices." );

	// Vertices can be loaded directly.
	m_vertexBuffer = std::make_shared<gl::Buffer>(uint32(_header.elementSize * _header.numElements), gl::Buffer::Usage::MAP_WRITE );
	void* dest = m_vertexBuffer->Map();
	//_file.seekg( _header.elementSize * _header.numElements, std::ios_base::cur );
	_file.read( (char*)dest, _header.elementSize * _header.numElements );
	if(_file.fail()) LOG_ERROR("Why?");
	m_vertexBuffer->Unmap();
}

void Scene::LoadTriangles( std::ifstream& _file, const FileDecl::NamedArray& _header )
{
	// Triangles must be converted: the material id must be added from another chunk.
	m_numTrianglesPerLeaf = _header.elementSize/sizeof(FileDecl::Triangle);
	m_triangleBuffer = std::make_shared<gl::Buffer>( uint32(m_numTrianglesPerLeaf * sizeof(Triangle) * _header.numElements), gl::Buffer::Usage::MAP_WRITE );
	Triangle* dest = (Triangle*)m_triangleBuffer->Map();
	// Mapping file<->GPU currently equal, read directly
	Assert(sizeof(Triangle) == sizeof(FileDecl::Triangle), "File or triangle format changed. Load buffered!");
	_file.read( (char*)dest, _header.numElements * _header.elementSize );
	m_triangleBuffer->Unmap();
}

void Scene::LoadMatRef( std::ifstream& _file, const Jo::Files::MetaFileWrapper::Node& _materials, const FileDecl::NamedArray& _header )
{
	Assert( _header.elementSize == 32, "Material reference seems to be changed!" );
	char name[32];
	for(uint i = 0; i < _header.numElements; ++i)
	{
		_file.read( name, _header.elementSize );
		std::string stdName = name;
		// Directly load from json now!
		const Jo::Files::MetaFileWrapper::Node* material;
		if(_materials.HasChild( stdName, &material ))
		{
			// Fill a structure for GPU
			LoadMaterial(*material);
		} else
			LOG_ERROR("Material '" + stdName + "' referenced but not found!");
	}
}

template<typename BVType>
void LoadHierachyHelper(char* _dest, std::ifstream& _file, const FileDecl::NamedArray& _header)
{
	Scene::TreeNode<BVType>* dest = reinterpret_cast<Scene::TreeNode<BVType>*>(_dest);
	for (uint32_t i = 0; i < _header.numElements; ++i, ++dest)
	{
		FileDecl::Node node;
		_file.read((char*)&node, sizeof(FileDecl::Node));

		dest->firstChild = node.firstChild;
		dest->escape = node.escape;
	}
}

void Scene::LoadHierarchy( std::ifstream& _file, const FileDecl::NamedArray& _header )
{
	// Read element wise and copy the tree structure
	char* dest = (char*)m_hierarchyBuffer->Map();
	
	if (m_bvType == ε::Types3D::BOX)
		LoadHierachyHelper<ei::Box>(dest, _file, _header);
	else if (m_bvType == ε::Types3D::SPHERE)
		LoadHierachyHelper<ei::Sphere>(dest, _file, _header);
	else
		LOG_ERROR("Unimplemented bvh type!");

	m_hierarchyBuffer->Unmap();
}

void Scene::LoadBoundingVolumes( std::ifstream& _file, const FileDecl::NamedArray& _header )
{
	// Read element wise and copy the volume information
	if( m_bvType == ε::Types3D::BOX )
	{
		TreeNode<ε::Box>* dest = (TreeNode<ε::Box>*)m_hierarchyBuffer->Map();
		for (uint32_t i = 0; i < _header.numElements; ++i, ++dest)
		{
			ε::Box buffer;
			_file.read(reinterpret_cast<char*>(&buffer), sizeof(ε::Box));
			dest->max = buffer.max;
			dest->min = buffer.min;
		}
	}
	else if( m_bvType == ε::Types3D::SPHERE )
	{
		TreeNode<ε::Sphere>* dest = (TreeNode<ε::Sphere>*)m_hierarchyBuffer->Map();
		for (uint32_t i = 0; i < _header.numElements; ++i, ++dest)
		{
			ε::Sphere buffer;
			_file.read(reinterpret_cast<char*>(&buffer), sizeof(ε::Sphere));
			dest->center = buffer.center;
			dest->radius = buffer.radius;
		}
	}
	else
		LOG_ERROR("Unimplemented bvh type!");
	m_hierarchyBuffer->Unmap();
}

void Scene::LoadMaterial( const Jo::Files::MetaFileWrapper::Node& _material )
{
	Material mat;
	// "String pool"
	static const std::string s_emissivity("emissivity");
	static const std::string s_refrN("refractionIndexN");
	static const std::string s_refrK("refractionIndexK");
	static const std::string s_diffuse("diffuse");
	static const std::string s_diffuseTex("diffuseTex");
	static const std::string s_opacity("opacity");
	static const std::string s_opacityTex("opacityTex");
	static const std::string s_specular("reflectiveness");
	static const std::string s_specularTex("reflectivenessTex");
	try {
		// Emissivity
		mat.emissivityRG.r = _material[s_emissivity][0].Get( 0.0f );
		mat.emissivityRG.g = _material[s_emissivity][1].Get( 0.0f );
		mat.emissivityB = _material[s_emissivity][2].Get( 0.0f );
		// Refraction parameters for Fresnel
		ε::Vec3 n(_material[s_refrN][0].Get(1.0f), _material[s_refrN][1].Get(1.0f), _material[s_refrN][2].Get(1.0f));
		ε::Vec3 k(_material[s_refrK][0].Get(0.0f), _material[s_refrK][1].Get(0.0f), _material[s_refrK][2].Get(0.0f));
		ε::Vec3 den = 1.0f / (sq(n+1.0f) + sq(k));
		mat.refractionIndexAvg = avg(n);
		mat.fresnel0 = (sq(n-1.0f) + sq(k)) * den;
		mat.fresnel1 = 4.0f * n * den;
		// Load or create diffuse texture
		if(_material.HasChild(s_diffuseTex))
			mat.diffuseTexHandle = GetBindlessHandle(_material[s_diffuseTex]);
		else mat.diffuseTexHandle = GetBindlessHandle(
			ε::Vec3(_material[s_diffuse][0].Get(0.5f), _material[s_diffuse][1].Get(0.5f), _material[s_diffuse][2].Get(0.5f)));
		// Load or create opacity texture
		if(_material.HasChild(s_opacityTex))
			mat.opacityTexHandle = GetBindlessHandle(_material[s_opacityTex]);
		else mat.opacityTexHandle = GetBindlessHandle(
			ε::Vec3(_material[s_opacity][0].Get(1.0f), _material[s_opacity][1].Get(1.0f), _material[s_opacity][2].Get(1.0f)));
		// Load or create specular texture
		if(_material.HasChild(s_specularTex))
			mat.opacityTexHandle = GetBindlessHandle(_material[s_specularTex]);
		else mat.opacityTexHandle = GetBindlessHandle(
			ε::Vec4(_material[s_specular][0].Get(1.0f), _material[s_specular][1].Get(1.0f), _material[s_specular][2].Get(1.0f), _material[s_specular][3].Get(1.0f)));
	} catch(const std::string& _msg ) {
		LOG_ERROR("Failed to load the material. Exception: " + _msg);
	} catch(...) {
		LOG_ERROR("Failed to load the material. Material file or textures corrupted (unknown exception).");
	}
	m_materials.push_back( mat );
}

uint64 Scene::GetBindlessHandle( const std::string& _name )
{
	auto it = m_textures.find(_name);
	if( it == m_textures.end() )
	{
		it = m_textures.insert( std::pair<std::string, std::unique_ptr<gl::Texture2D>>(
			_name, gl::Texture2D::LoadFromFile(_name, false)) ).first;
	}
	
	return GL_RET_CALL(glGetTextureSamplerHandleARB, it->second->GetInternHandle(), m_samplerLinearNoMipMap.GetInternSamplerId());
}

uint64 Scene::GetBindlessHandle( const ε::Vec3& _data )
{
	std::string genName = "gen" + std::to_string(_data.x) + '_' + std::to_string(_data.y) + '_' + std::to_string(_data.z);
	auto it = m_textures.find(genName);
	if( it == m_textures.end() )
	{
		it = m_textures.insert( std::pair<std::string, std::unique_ptr<gl::Texture2D>>(
			genName, std::unique_ptr<gl::Texture2D>(new gl::Texture2D(1, 1, gl::TextureFormat::RGB8, &_data, gl::TextureSetDataFormat::RGB, gl::TextureSetDataType::FLOAT))) ).first;
	}
	
	return GL_RET_CALL(glGetTextureSamplerHandleARB, it->second->GetInternHandle(), m_samplerLinearNoMipMap.GetInternSamplerId());
}

uint64 Scene::GetBindlessHandle( const ε::Vec4& _data )
{
	std::string genName = "gen" + std::to_string(_data.x) + '_' + std::to_string(_data.y) + '_' + std::to_string(_data.z) + '_' + std::to_string(_data.w);
	auto it = m_textures.find(genName);
	if( it == m_textures.end() )
	{
		it = m_textures.insert( std::pair<std::string, std::unique_ptr<gl::Texture2D>>(
			genName, std::unique_ptr<gl::Texture2D>(new gl::Texture2D(1, 1, gl::TextureFormat::RGBA32F, &_data, gl::TextureSetDataFormat::RGBA, gl::TextureSetDataType::FLOAT))) ).first;
	}
	
	return GL_RET_CALL(glGetTextureSamplerHandleARB, it->second->GetInternHandle(), m_samplerLinearNoMipMap.GetInternSamplerId());
}

void Scene::LoadLightSources( std::ifstream& _file )
{
	// TODO somehow read the buffers??
	/*for(uint32_t i = 0; i < _header.numElements * m_numTrianglesPerLeaf; ++i,++dest)
	{
		// Mapping file<->GPU currently equal, read directly
		Assert(sizeof(Triangle) == sizeof(FileDecl::Triangle), "File or triangle format changed. Load buffered!");
		_file.read( (char*)dest, sizeof(Triangle) );
		// Is this a light source triangle?
		if( any(m_materials[dest->material].emissivityRG >= 0.0f)
			|| (m_materials[dest->material].emissivityB >= 0.0f) )
		{
			LightTriangle lightSource;
			lightSource.luminance = ε::Vec3(m_materials[dest->material].emissivityRG, m_materials[dest->material].emissivityB);
			// Get the 3 vertices from file
			size_t oldOffset = _file.tellg();
			for(int j = 0; j < 3; ++j)
			{
				_file.seekg(m_fileVertexOffset + dest->vertices[i] * sizeof(Vertex));
				Vertex v;
				_file.read( (char*)&v, sizeof(Vertex) );
				lightSource.position[i] = v.position;
				lightSource.normal[i] = v.normal;
			}
			//lightSource.normal[0] = ;
			_file.seekg(oldOffset);
			m_lightTriangles.push_back(lightSource);
		}
	}*/
}
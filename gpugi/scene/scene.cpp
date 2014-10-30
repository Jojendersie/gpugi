#include "scene.hpp"
#include "../glhelper/gl.hpp"
#include "../utilities/logger.hpp"
#include "../utilities/assert.hpp"

#include <fstream>

Scene::Scene( const std::string& _file ) :
	m_numTrianglesPerLeaf(0),
	m_bvType( ε::Types3D::NUM_TYPES )	// invalid type so far
{
	std::ifstream file( _file );
	if( file.bad() )
		LOG_ERROR( "Cannot open scene file \"" + _file + "\"!" );

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
		m_hierarchyBuffer = std::make_shared<gl::Buffer>(gl::Buffer( sizeof(TreeNode<ε::Box>) * m_numInnerNodes, gl::Buffer::Usage::WRITE ));
	else if( m_bvType == ε::Types3D::SPHERE )
		m_hierarchyBuffer = std::make_shared<gl::Buffer>(gl::Buffer( sizeof(TreeNode<ε::Sphere>) * m_numInnerNodes, gl::Buffer::Usage::WRITE ));
    
	// Read one data array after another. The order is undefined.
	file.seekg( 0 );
	while( !file.eof() )
	{
		FileDecl::NamedArray header;
		file.read( reinterpret_cast<char*>(&header), sizeof(header) );
		if(strcmp(header.name, "vertices") == 0) LoadVertices(file, header);
		else if(strcmp(header.name, "triangles") == 0) LoadTriangles(file, header);
		else if(strcmp(header.name, "materialref") == 0) LoadMatRef(file, header);
		else if(strcmp(header.name, "materialassociation") == 0) LoadMatAssocialtion(file, header);
		else if(strcmp(header.name, "hierarchy") == 0) LoadHierarchy(file, header);
		else if(strcmp(header.name, "bounding_aabox") == 0) LoadBoundingVolumes(file, header);
		else if(strcmp(header.name, "bounding_sphere") == 0) LoadBoundingVolumes(file, header);
	}

	if( m_numTrianglesPerLeaf == 0 )
		LOG_ERROR("Did not find a \"triangles\" buffer in" + _file + "!");
	if( m_vertexBuffer == nullptr )
		LOG_ERROR("Did not find a \"vertices\" buffer in" + _file + "!");
}

Scene::~Scene()
{
}

void Scene::LoadVertices( std::ifstream& _file, const FileDecl::NamedArray& _header )
{
	Assert( _header.elementSize == sizeof(Vertex), "Format seems to contain other vertices." );

	// Vertices can be loaded directly.
	m_vertexBuffer = std::make_shared<gl::Buffer>(gl::Buffer( _header.elementSize * _header.numElements, gl::Buffer::Usage::WRITE ));
	void* dest = m_vertexBuffer->Map();
	_file.read( (char*)dest, _header.elementSize * _header.numElements );
	m_vertexBuffer->Unmap();
}

void Scene::LoadTriangles( std::ifstream& _file, const FileDecl::NamedArray& _header )
{
	// Triangles must be converted: the material id must be added from another chunk.
	m_numTrianglesPerLeaf = _header.elementSize/sizeof(FileDecl::Triangle);
	if( m_triangleBuffer == nullptr )
	{
		m_triangleBuffer = std::make_shared<gl::Buffer>(gl::Buffer( m_numTrianglesPerLeaf * sizeof(Triangle) * _header.numElements, gl::Buffer::Usage::WRITE ));
	} else
		Assert( m_triangleBuffer->GetSize() == m_numTrianglesPerLeaf * sizeof(Triangle) * _header.numElements, "The first defined triangle count differs from the current one!" );
	Triangle* dest = (Triangle*)m_triangleBuffer->Map();
	for(uint32_t i = 0; i < _header.numElements * m_numTrianglesPerLeaf; ++i,++dest)
	{
		FileDecl::Triangle triangle;
		_file.read( (char*)&triangle, sizeof(FileDecl::Triangle) );
		dest->vertices[0] = triangle.vertices[0];
		dest->vertices[1] = triangle.vertices[1];
		dest->vertices[2] = triangle.vertices[2];
	}
	m_triangleBuffer->Unmap();
}

void Scene::LoadMatRef( std::ifstream& _file, const FileDecl::NamedArray& _header )
{
	Assert( _header.elementSize == 32, "Material reference seems to be changed!" );
	char name[32];
	for(uint i = 0; i < _header.numElements; ++i)
	{
		_file.read( name, _header.elementSize );
		// TODO directly load from json now!
	}
}

void Scene::LoadMatAssocialtion( std::ifstream& _file, const FileDecl::NamedArray& _header )
{
	// Triangles must be converted: the material id must be added from another chunk.
	if( m_triangleBuffer == nullptr )
	{
		m_triangleBuffer = std::make_shared<gl::Buffer>(gl::Buffer( sizeof(Triangle) * _header.numElements, gl::Buffer::Usage::WRITE ));
	} else
		Assert( m_triangleBuffer->GetSize() == sizeof(Triangle) * _header.numElements, "The first defined triangle count differs from the current one!" );
	Triangle* dest = (Triangle*)m_triangleBuffer->Map();
	for(uint32_t i = 0; i < _header.numElements; ++i,++dest)
	{
		uint16 materialID;
		_file.read( (char*)&materialID, 2 );
		dest->material = materialID;
	}
	m_triangleBuffer->Unmap();
}

void Scene::LoadHierarchy( std::ifstream& _file, const FileDecl::NamedArray& _header )
{
	// Read element wise and copy the tree structure
	TreeNode<ε::Box>* dest = (TreeNode<ε::Box>*)m_hierarchyBuffer->Map();
	for(uint32_t i = 0; i < _header.numElements; ++i,++dest)
	{
		FileDecl::Node node;
		_file.read( (char*)&node, sizeof(FileDecl::Node) );
		dest->firstChild = node.firstChild;
		dest->escape = node.escape;
	}
	m_hierarchyBuffer->Unmap();
}

void Scene::LoadBoundingVolumes( std::ifstream& _file, const FileDecl::NamedArray& _header )
{
	// Read element wise and copy the volume information
	if( m_bvType == ε::Types3D::BOX )
	{
		TreeNode<ε::Box>* dest = (TreeNode<ε::Box>*)m_hierarchyBuffer->Map();
		for(uint32_t i = 0; i < _header.numElements; ++i,++dest)
			_file.read( (char*)&dest->boundingVolume, sizeof(ε::Box) );
	} else if( m_bvType == ε::Types3D::SPHERE )
	{
		TreeNode<ε::Sphere>* dest = (TreeNode<ε::Sphere>*)m_hierarchyBuffer->Map();
		for(uint32_t i = 0; i < _header.numElements; ++i,++dest)
			_file.read( (char*)&dest->boundingVolume, sizeof(ε::Sphere) );
	}
	m_hierarchyBuffer->Unmap();
}

#include "scene.hpp"
#include "../glhelper/gl.hpp"
#include "../utilities/logger.hpp"
#include "../utilities/assert.hpp"

#include <fstream>

Scene::Scene( const std::string& _file )
{
	std::ifstream file( _file );
	if( file.bad() )
		LOG_ERROR( "Cannot open scene file \"" + _file + "\"!" );
    
	// Read one data array after another. The order is undefined.
	while( !file.eof() )
	{
		FileDecl::NamedArray header;
		file.read( reinterpret_cast<char*>(&header), sizeof(header) );
		if(strcmp(header.name, "vertices") == 0) LoadVertices(file, header);
		else if(strcmp(header.name, "triangles") == 0) LoadTriangles(file, header);
	}
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
	if( m_triangleBuffer == nullptr )
	{
		m_triangleBuffer = std::make_shared<gl::Buffer>(gl::Buffer( sizeof(Triangle) * _header.numElements, gl::Buffer::Usage::WRITE ));
		m_numTrianglesPerLeaf = _header.elementSize/sizeof(FileDecl::Triangle);
	} else
		Assert( m_numTrianglesPerLeaf == _header.elementSize/sizeof(FileDecl::Triangle), "The first defined triangle count differs from the currend one!" );
	Triangle* dest = (Triangle*)m_vertexBuffer->Map();
	for(uint32_t i = 0; i < _header.numElements * m_numTrianglesPerLeaf; ++i,++dest)
	{
		FileDecl::Triangle triangle;
		_file.read( (char*)&triangle, sizeof(FileDecl::Triangle) );
		dest->vertices[0] = triangle.vertices[0];
		dest->vertices[1] = triangle.vertices[1];
		dest->vertices[2] = triangle.vertices[2];
	}
	m_vertexBuffer->Unmap();
}

#include "bvhmake.hpp"
#include "filedef.hpp"
#include <assimp/matrix4x4.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <fstream>
#include "../gpugi/utilities/assert.hpp"

// Helper method to transform Assimp types into something useful.
template<typename T, typename F>
T hard_cast(F _from)
{
	static_assert(sizeof(T) == sizeof(F), "Cannot cast types of different sizes");
	return *(T*)&_from;
}


void foo() {}

BVHBuilder::BVHBuilder()
{
    // Register methods
    m_buildMethods.insert( {"kdtree", foo} );
    m_fitMethods.insert( {"aabox", foo} );

    // Set defaults
    m_buildMethod = m_buildMethods["kdtree"];
    m_fitMethod = m_fitMethods["aabox"];
}

std::string BVHBuilder::GetBuildMethods()
{
    std::string list = "";
    for( auto it : m_buildMethods )
    {
        if( !list.empty() ) list += ", ";
        list += it.first;
    }
    return list;
}

std::string BVHBuilder::GetFitMethods()
{
    std::string list = "";
    for( auto it : m_fitMethods )
    {
        if( !list.empty() ) list += ", ";
        list += it.first;
    }
    return list;
}


bool BVHBuilder::SetBuildMethod( const char* _methodName )
{
    auto it = m_buildMethods.find( _methodName );
    if( it != m_buildMethods.end() )
        m_buildMethod = it->second;
    return it != m_fitMethods.end();
}

bool BVHBuilder::SetGeometryType( const char* _methodName )
{
    auto it = m_fitMethods.find( _methodName );
    if( it != m_fitMethods.end() )
        m_buildMethod = it->second;
    return it != m_fitMethods.end();
}

bool BVHBuilder::LoadSceneWithAssimp( const char* _file )
{
    // Ignore line/point primitives
	m_importer.SetPropertyInteger( AI_CONFIG_PP_SBP_REMOVE,
		aiPrimitiveType_POINT | aiPrimitiveType_LINE );

	m_scene = m_importer.ReadFile( _file,
		aiProcess_CalcTangentSpace		|
		aiProcess_Triangulate			|
		aiProcess_GenSmoothNormals		|	// TODO: is angle 80% already set?
		aiProcess_ValidateDataStructure	|
		aiProcess_SortByPType			|
		aiProcess_RemoveRedundantMaterials	|
		//aiProcess_FixInfacingNormals	|
		aiProcess_FindInvalidData		|
		aiProcess_GenUVCoords			|
		aiProcess_TransformUVCoords     |
        aiProcess_ImproveCacheLocality  |
        aiProcess_JoinIdenticalVertices
		//aiProcess_FlipUVs
	);

	return m_scene != nullptr;
}


void BVHBuilder::ExportGeometry( std::ofstream& _file )
{
    // Prepare file headers and find out how much space is required
    NamedArray indexHeader;
    NamedArray vertexHeader;
    strcpy( indexHeader.name, "triangles" );
    strcpy( vertexHeader.name, "vertices" );
    indexHeader.elementSize = sizeof(Triangle);
    vertexHeader.elementSize = sizeof(Vertex);
    indexHeader.numElements = 0;
    vertexHeader.numElements = 0;
    CountGeometry( m_scene->mRootNode, vertexHeader.numElements, indexHeader.numElements );

    // Allocate space for positions and index buffer (these are required for
    // the hierarchy build algorithm).
    m_positions.reset( new ε::Vec3[vertexHeader.numElements] );
    m_triangles.reset( new uint[indexHeader.numElements * 3] );
    m_triangleCursor = 0;
    // TODO Materials

    // Export vertices directly streams to the file and fills m_positions
    // as well as m_triangles
    _file.write( (const char*)&vertexHeader, sizeof(NamedArray) );
    ExportVertices( _file, m_scene->mRootNode, ε::identity4x4() );
    _file.write( (const char*)&indexHeader, sizeof(NamedArray) );
    _file.write( (const char*)&m_triangles[0], sizeof(uint) * indexHeader.numElements * 3 );

    // This is not required anymore, make place for the algorithms
    m_importer.FreeScene();
}

void BVHBuilder::CountGeometry( const struct aiNode* _node, uint& _numVertices, uint& _numFaces )
{
    // Count the meshes from this node
    for( unsigned i = 0; i < _node->mNumMeshes; ++i )
	{
		const aiMesh* mesh = m_scene->mMeshes[ _node->mMeshes[i] ];
        _numFaces += mesh->mNumFaces;
        _numVertices += mesh->mNumVertices;
    }

    // Count all children
	for( unsigned i = 0; i < _node->mNumChildren; ++i )
		CountGeometry( _node->mChildren[i], _numVertices, _numFaces );
}

void BVHBuilder::ExportVertices( std::ofstream& _file,
    const struct aiNode* _node,
    const ε::Mat4x4& _transformation)
{
    ε::Mat4x4 transformation = hard_cast<ε::Mat4x4>(_node->mTransformation) * _transformation;
    for( unsigned i = 0; i < _node->mNumMeshes; ++i )
	{
		const aiMesh* mesh = m_scene->mMeshes[ _node->mMeshes[i] ];
        // TODO create a material entry

        // Add each triangle to the output array
		for( unsigned t = 0; t < mesh->mNumFaces; ++t )
		{
			const aiFace& face = mesh->mFaces[t];
			Assert( face.mNumIndices == 3, "This is a triangle importer!" );
            m_triangles[m_triangleCursor + 0] = face.mIndices[0] + m_vertexCursor;
            m_triangles[m_triangleCursor + 1] = face.mIndices[1] + m_vertexCursor;
            m_triangles[m_triangleCursor + 2] = face.mIndices[2] + m_vertexCursor;
            m_triangleCursor += 3;
        }

        // Add vertices to file and positions to the list
        for( unsigned v = 0; v < mesh->mNumVertices; ++v )
        {
            Vertex vertex;
            vertex.position = ε::Vec3(transformation * homo(hard_cast<ε::Vec3>(mesh->mVertices[v])));
            vertex.normal = hard_cast<ε::Vec3>(mesh->mNormals[v]);
            if( mesh->HasTextureCoords(0) )
				vertex.texcoord = ε::Vec2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
			else
                vertex.texcoord = ε::Vec2(0.0f, 0.0f);
            _file.write( (const char*)&vertex, sizeof(Vertex) );
            m_positions[m_vertexCursor++] = vertex.position;
        }
    }

    // Export all children
	for( unsigned i = 0; i < _node->mNumChildren; ++i )
        ExportVertices( _file, _node->mChildren[i], transformation );
}
#include "bvhmake.hpp"
#include <assimp/matrix4x4.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

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
		aiProcess_TransformUVCoords
		//aiProcess_FlipUVs
	);

	return m_scene != nullptr;
}
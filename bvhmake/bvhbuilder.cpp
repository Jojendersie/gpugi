#include "bvhmake.hpp"
#include "filedef.hpp"
#include "fitmethods/aaboxfit.hpp"
#include "fitmethods/aaellipsoidfit.hpp"
#include "buildmethods/kdtree.hpp"
#include "buildmethods/sweep.hpp"
#include "../gpugi/utilities/assert.hpp"
#include <assimp/matrix4x4.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <fstream>
#include <stack>
#include <iostream>

using namespace Jo::Files;

// Helper method to transform Assimp types into something useful.
template<typename T, typename F>
T hard_cast(F _from)
{
	static_assert(sizeof(T) == sizeof(F), "Cannot cast types of different sizes");
	return *(T*)&_from;
}


const FileDecl::Triangle FileDecl::INVALID_TRIANGLE = {{0x0, 0x0, 0x0}};


BVHBuilder::BVHBuilder() :
    m_bvbuffer(nullptr),
    m_nodes(nullptr),
    m_leaves(nullptr),
    m_innerNodeCount(0),
    m_maxInnerNodeCount(0),
    m_leafNodeCount(0),
    m_maxLeafNodeCount(0)
{
    // Register methods
    m_buildMethods.insert( {"kdtree", new BuildKdtree(this)} );
	m_buildMethods.insert( {"sweep", new BuildSweep(this)} );
    m_fitMethods.insert( {"aabox", new FitBox(this)} );
	m_fitMethods.insert( {"ellipsoid", new FitEllipsoid(this)} );

    // Set defaults
    m_buildMethod = m_buildMethods["kdtree"];
    m_fitMethod = m_fitMethods["aabox"];
}

BVHBuilder::~BVHBuilder()
{
    for( auto it : m_buildMethods )
        delete it.second;

    for( auto it : m_fitMethods )
        delete it.second;

    delete[] m_bvbuffer;
    delete[] m_nodes;
    delete[] m_leaves;
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
    return it != m_buildMethods.end();
}

bool BVHBuilder::SetGeometryType( const char* _methodName )
{
    auto it = m_fitMethods.find( _methodName );
    if( it != m_fitMethods.end() )
        m_fitMethod = it->second;
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


void BVHBuilder::LoadMaterials( const std::string& _materialFileName )
{
	if( Utils::Exists(_materialFileName) )
	{
		// Simply read in the file. It is kept as it is.
		HDDFile file( _materialFileName );
		m_materials.Read( file );
		std::cerr << "Loaded " << _materialFileName << std::endl;
	} else
		std::cerr << "No json material file yet." << std::endl;
}


void BVHBuilder::ExportGeometry( std::ofstream& _file )
{
    // Prepare file headers and find out how much space is required
    FileDecl::NamedArray vertexHeader;
	FileDecl::NamedArray tangentsHeader;
    strcpy( vertexHeader.name, "vertices" );
	strcpy( tangentsHeader.name, "tangents" );
    vertexHeader.elementSize = sizeof(FileDecl::Vertex);
	tangentsHeader.elementSize = sizeof(ε::Vec3);
    uint32 numTriangles = 0;
    vertexHeader.numElements = 0;
    CountGeometry( m_scene->mRootNode, vertexHeader.numElements, numTriangles );
	tangentsHeader.numElements = vertexHeader.numElements;

    // Allocate space for positions and index buffer (these are required for
    // the hierarchy build algorithm).
    m_positions.reset( new ε::Vec3[vertexHeader.numElements] );
    m_triangles.reset( new uint32[numTriangles * 4] );
    m_triangleCount = 0;

    // Export vertices directly streams to the file and fills m_positions
    // as well as m_triangles
    _file.write( (const char*)&vertexHeader, sizeof(FileDecl::NamedArray) );
    ExportVertices( _file, m_scene->mRootNode, ε::identity4x4() );

	// Export tangents
	_file.write( (const char*)&tangentsHeader, sizeof(FileDecl::NamedArray) );
	ExportTangents( _file, m_scene->mRootNode, ε::identity4x4() );

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
	ε::Mat3x3 invTransTransform(transformation.m00, transformation.m01, transformation.m02,
								transformation.m10, transformation.m11, transformation.m12,
								transformation.m20, transformation.m21, transformation.m22);
	invTransTransform = transpose(invert(invTransTransform));
    for( unsigned i = 0; i < _node->mNumMeshes; ++i )
	{
		const aiMesh* mesh = m_scene->mMeshes[ _node->mMeshes[i] ];

        // Find the material entry
		aiString aiName;
		m_scene->mMaterials[mesh->mMaterialIndex]->Get( AI_MATKEY_NAME, aiName );
		std::string materialName = aiName.C_Str();
		uint materialIndex = 0;
		while(materialIndex < m_materials.RootNode.Size()
			&& m_materials.RootNode[materialIndex].GetName() != materialName)
			++materialIndex;
		if( materialIndex == m_materials.RootNode.Size() )
			std::cerr << "Could not find the mesh material" << std::endl;

        // Add each triangle to the output array
		for( unsigned t = 0; t < mesh->mNumFaces; ++t )
		{
			const aiFace& face = mesh->mFaces[t];
			Assert( face.mNumIndices == 3, "This is a triangle importer!" );
            m_triangles[m_triangleCount*4 + 0] = face.mIndices[0] + m_vertexCount;
            m_triangles[m_triangleCount*4 + 1] = face.mIndices[1] + m_vertexCount;
            m_triangles[m_triangleCount*4 + 2] = face.mIndices[2] + m_vertexCount;
			m_triangles[m_triangleCount*4 + 3] = materialIndex;
            ++m_triangleCount;
        }

        // Add vertices to file and positions to the list
        for( unsigned v = 0; v < mesh->mNumVertices; ++v )
        {
            FileDecl::Vertex vertex;
            vertex.position = ε::Vec3(transformation * homo(hard_cast<ε::Vec3>(mesh->mVertices[v])));
            vertex.normal = invTransTransform * hard_cast<ε::Vec3>(mesh->mNormals[v]);
            if( mesh->HasTextureCoords(0) )
				vertex.texcoord = ε::Vec2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
			else
                vertex.texcoord = ε::Vec2(0.0f, 0.0f);
            _file.write( (const char*)&vertex, sizeof(FileDecl::Vertex) );
            m_positions[m_vertexCount++] = vertex.position;
        }
    }

    // Export all children
	for( unsigned i = 0; i < _node->mNumChildren; ++i )
        ExportVertices( _file, _node->mChildren[i], transformation );
}


void BVHBuilder::ExportTangents( std::ofstream& _file,
    const struct aiNode* _node,
    const ε::Mat4x4& _transformation)
{
    ε::Mat4x4 transformation = hard_cast<ε::Mat4x4>(_node->mTransformation) * _transformation;
	ε::Mat3x3 invTransTransform(transformation.m00, transformation.m01, transformation.m02,
								transformation.m10, transformation.m11, transformation.m12,
								transformation.m20, transformation.m21, transformation.m22);
	invTransTransform = transpose(invert(invTransTransform));
    for( unsigned i = 0; i < _node->mNumMeshes; ++i )
	{
		const aiMesh* mesh = m_scene->mMeshes[ _node->mMeshes[i] ];

        // Add vertex tangents to file
        for( unsigned v = 0; v < mesh->mNumVertices; ++v )
        {
            ε::Vec3 tangent = invTransTransform * hard_cast<ε::Vec3>(mesh->mTangents[v]);
            _file.write( (const char*)&tangent, sizeof(ε::Vec3) );
        }
    }

    // Export all children
	for( unsigned i = 0; i < _node->mNumChildren; ++i )
        ExportTangents( _file, _node->mChildren[i], transformation );
}


void BVHBuilder::BuildBVH()
{
    // The maximum number of nodes is limited. It is possible that a build
    // method use less, but should never take more.
    m_buildMethod->EstimateNodeCounts(m_maxInnerNodeCount, m_maxLeafNodeCount);
    m_innerNodeCount = m_leafNodeCount = 0;
    m_nodes = new Node[m_maxInnerNodeCount];
    m_leaves = new FileDecl::Leaf[m_maxLeafNodeCount];
    switch(m_fitMethod->Type())
    {
    case FitMethod::BVType::AABOX:
        m_bvbuffer = new ε::Box[m_maxInnerNodeCount];
        break;
	case FitMethod::BVType::SPHERE:
        m_bvbuffer = new ε::Sphere[m_maxInnerNodeCount];
        break;
    case FitMethod::BVType::AAELLIPSOID:
        m_bvbuffer = new ε::Ellipsoid[m_maxInnerNodeCount];
        break;
    default:
        break;
    }

    // Build now
    uint32 root = (*m_buildMethod)();
	Assert( root == 0, "The root must be always the first node! Resort or allocte in perorder." );
}

void BVHBuilder::ExportBVH( std::ofstream& _file )
{
    // Prepare file headers and find out how much space is required
    FileDecl::NamedArray treeHeader;
    FileDecl::NamedArray indexHeader;
    FileDecl::NamedArray bvHeader;
    strcpy( treeHeader.name, "hierarchy" );
    strcpy( indexHeader.name, "triangles" );
    treeHeader.elementSize = sizeof(FileDecl::Node);
    indexHeader.elementSize = sizeof(FileDecl::Triangle) * FileDecl::Leaf::NUM_PRIMITIVES;
    treeHeader.numElements = m_innerNodeCount;
    indexHeader.numElements = m_leafNodeCount;
    bvHeader.numElements = m_innerNodeCount;
    switch(m_fitMethod->Type())
    {
    case FitMethod::BVType::AABOX: strcpy( bvHeader.name, "bounding_aabox" );
        bvHeader.elementSize = sizeof(ε::Box);
        break;
    case FitMethod::BVType::SPHERE: strcpy( bvHeader.name, "bounding_sphere" );
        bvHeader.elementSize = sizeof(ε::Sphere);
        break;
    case FitMethod::BVType::AAELLIPSOID: strcpy( bvHeader.name, "bounding_ellipsoid" );
        bvHeader.elementSize = sizeof(ε::Ellipsoid);
        break;
    default: Assert( false, "Current geometry cannot be stored!" ); break;
    }

	// Write the bounding volumes sequentially
    _file.write( (const char*)&bvHeader, sizeof(FileDecl::NamedArray) );
    _file.write( (const char*)m_bvbuffer, bvHeader.elementSize * bvHeader.numElements );

	// Write the hierarchy as it is in the memory
    _file.write( (const char*)&treeHeader, sizeof(FileDecl::NamedArray) );
	RecursiveWriteHierarchy( _file, 0, 0, 0 );

    // Write a "resorted index buffer" to file
    _file.write( (const char*)&indexHeader, sizeof(FileDecl::NamedArray) );
    _file.write( (const char*)m_leaves, indexHeader.elementSize * indexHeader.numElements );
}


void BVHBuilder::ExportMaterials( std::ofstream& _file, const std::string& _materialFileName )
{
	if( !m_scene->HasMaterials() )
	{
		std::cerr << "The scene has no materials to export!";
		return;
	}

	for( uint i = 0; i < m_scene->mNumMaterials; ++i )
	{
		// Get name and check if the material was imported before
		aiString aiName;
		auto mat = m_scene->mMaterials[i];
		mat->Get( AI_MATKEY_NAME, aiName );
		std::string name = aiName.C_Str();
		if( m_materials.RootNode.HasChild( name ) )
			continue;

		// The current material was not imported before do it now
		auto& matNode = m_materials.RootNode.Add( name, MetaFileWrapper::ElementType::NODE, 0 );
		// Load diffuse
		if( mat->GetTexture( aiTextureType_DIFFUSE, 0, &aiName ) == AI_SUCCESS )
		{
			matNode[std::string("diffuseTex")] = aiName.C_Str();
		} else {
			auto& diff = matNode.Add( "diffuse", MetaFileWrapper::ElementType::FLOAT, 3 );
			aiColor3D color = aiColor3D( 0.0f, 0.0f, 0.0f );
			mat->Get( AI_MATKEY_COLOR_DIFFUSE, color );
			diff[0] = color.r;
			diff[1] = color.g;
			diff[2] = color.b;
		}

		// Load specular
		if( mat->GetTexture( aiTextureType_SPECULAR, 0, &aiName ) == AI_SUCCESS )
		{
			matNode[std::string("reflectivenessTex")] = aiName.C_Str();
		} else
		if( mat->GetTexture( aiTextureType_SHININESS, 0, &aiName ) == AI_SUCCESS )
		{
			matNode[std::string("reflectivenessTex")] = aiName.C_Str();
		} else {
			auto& refl = matNode.Add( "reflectiveness", MetaFileWrapper::ElementType::FLOAT, 4 );
			aiColor3D color = aiColor3D( 0.0f, 0.0f, 0.0f );
			float shininess = 1.0f;
			if( mat->Get( AI_MATKEY_COLOR_REFLECTIVE, color ) != AI_SUCCESS )
				mat->Get( AI_MATKEY_COLOR_SPECULAR, color );
			mat->Get( AI_MATKEY_SHININESS, shininess );
			refl[0] = color.r;
			refl[1] = color.g;
			refl[2] = color.b;
			refl[3] = shininess;
		}

		// Without knowledge use simple defaults
		auto& refrN = matNode.Add( "refractionIndexN", MetaFileWrapper::ElementType::FLOAT, 3 );
		auto& refrR = matNode.Add( "refractionIndexK", MetaFileWrapper::ElementType::FLOAT, 3 );
		refrN[0] = refrN[1] = refrN[2] = 1.45f;
		refrR[0] = refrR[1] = refrR[2] = 0.0f;

		// Load opacity
		if( mat->GetTexture( aiTextureType_OPACITY, 0, &aiName ) == AI_SUCCESS )
		{
			matNode[std::string("opacityTex")] = aiName.C_Str();
		} else {
			auto& opactiy = matNode.Add( "opacity", MetaFileWrapper::ElementType::FLOAT, 3 );
			float value = 1.0f;
			mat->Get( AI_MATKEY_OPACITY, value );
			opactiy[0] = value;
			opactiy[1] = value;
			opactiy[2] = value;
		}

		// Load emissivity
		{
			auto& emissivity = matNode.Add( "emissivity", MetaFileWrapper::ElementType::FLOAT, 3 );
			aiColor3D color = aiColor3D( 0.0f, 0.0f, 0.0f );
			mat->Get( AI_MATKEY_COLOR_EMISSIVE, color );
			emissivity[0] = color.r;
			emissivity[1] = color.g;
			emissivity[2] = color.b;
		}
	}

	m_materials.Write( HDDFile(_materialFileName, HDDFile::OVERWRITE), Format::JSON );

	// Create the materialref table
	FileDecl::NamedArray materialHeader;
    strcpy( materialHeader.name, "materialref" );
    materialHeader.elementSize = sizeof(FileDecl::Material);
    materialHeader.numElements = static_cast<uint32>(m_materials.RootNode.Size());

    _file.write( (const char*)&materialHeader, sizeof(FileDecl::NamedArray) );
	for( uint i = 0; i < materialHeader.numElements; ++i )
	{
		size_t len = m_materials.RootNode[i].GetName().length();
		if( len > 31 ) {
			std::cerr << "Material name to long: shortening.";
		}
		FileDecl::Material mat;
		std::strncpy(mat.material, m_materials.RootNode[i].GetName().c_str(), 32);
		_file.write( (const char*)&mat, sizeof(FileDecl::Material) );
	}
}


ε::Triangle BVHBuilder::GetTriangle( uint32 _index ) const
{
    return ε::Triangle( m_positions[ m_triangles[_index * 4 + 0] ],
                        m_positions[ m_triangles[_index * 4 + 1] ],
                        m_positions[ m_triangles[_index * 4 + 2] ]
        );
}

ε::Triangle BVHBuilder::GetTriangle( FileDecl::Triangle _triangle ) const
{
    return ε::Triangle( m_positions[ _triangle.vertices[0] ],
                        m_positions[ _triangle.vertices[1] ],
                        m_positions[ _triangle.vertices[2] ]
        );
}

FileDecl::Triangle BVHBuilder::GetTriangleIdx( uint32 _index ) const
{
    FileDecl::Triangle t;
    t.vertices[0] = m_triangles[_index * 4 + 0];
    t.vertices[1] = m_triangles[_index * 4 + 1];
    t.vertices[2] = m_triangles[_index * 4 + 2];
	t.material = m_triangles[_index * 4 + 3];
    return t;
}

uint32 BVHBuilder::GetNewLeaf()
{
    Assert( m_leaves != nullptr, "BuildBVH() was not called or the memory is not allocated for other reasons." );
    Assert( m_leafNodeCount < m_maxLeafNodeCount, "Out-of-Bounds. The builder's estimation for the leaf count was to small!" );
    return m_leafNodeCount++;
}

uint32 BVHBuilder::GetNewNode()
{
    Assert( m_innerNodeCount < m_maxInnerNodeCount, "Out-of-Bounds. The builder's estimation for the inner node count was to small!" );
	Assert( !(m_innerNodeCount & 0x80000000), "Scene too large. The first bit is reserved as flag." );
    return m_innerNodeCount++;
}

void BVHBuilder::RecursiveWriteHierarchy( std::ofstream& _file, uint32 _this, uint32 _parent, uint32 _escape )
{
	FileDecl::Node node;
	node.firstChild = m_nodes[_this].left;
	node.parent = _parent;
	node.escape = _escape;
	_file.write( (const char*)&node, sizeof(FileDecl::Node) );
	if( !(m_nodes[_this].left & 0x80000000) )
	{
		RecursiveWriteHierarchy( _file, m_nodes[_this].left, _this, m_nodes[_this].right );
		RecursiveWriteHierarchy( _file, m_nodes[_this].right, _this, _escape );
	}

	// Use the processor stack to find the right escape pointer
/*	static uint32 escape = 0;
	// Push (oldEscape) is stored on the stack automatically and escape is the
	// top of the stack.
	uint32 oldEscape = escape;
	escape = m_nodes[_this].right;
	RecursiveWriteHierarchy( _file, m_nodes[_this].left, _this );
	// Pop
	escape = oldEscape;
	RecursiveWriteHierarchy( _file, m_nodes[_this].right, _this );*/
}
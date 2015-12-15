#include "bvhmake.hpp"
#include "filedef.hpp"
#include "fitmethods/aaboxfit.hpp"
#include "fitmethods/aaellipsoidfit.hpp"
#include "buildmethods/kdtree.hpp"
#include "buildmethods/sweep.hpp"
#include "buildmethods/lds.hpp"
#include "processing/tesselate.hpp"
#include "processing/approx_sggx.hpp"
#include "../gpugi/utilities/assert.hpp"
#include "../gpugi/utilities/logger.hpp"
#include <assimp/matrix4x4.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <fstream>
#include <stack>
#include <iostream>

using namespace Jo::Files;

// Needs the instance for hashing redundant vertices bidirectional and without
// doublication.
const BVHBuilder* g_bvhbuilder;

// Helper method to transform Assimp types into something useful.
template<typename T, typename F>
T hard_cast(F _from)
{
	static_assert(sizeof(T) == sizeof(F), "Cannot cast types of different sizes");
	return *(T*)&_from;
}


const FileDecl::Triangle FileDecl::INVALID_TRIANGLE = {{0x0, 0x0, 0x0}, 0};


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
	m_buildMethods.insert( {"lds", new BuildLDS(this)} );
	m_fitMethods.insert( {"aabox", new FitBox(this)} );
	m_fitMethods.insert( {"ellipsoid", new FitEllipsoid(this)} );

    // Set defaults
    m_buildMethod = m_buildMethods["kdtree"];
    m_fitMethod = m_fitMethods["aabox"];

	g_bvhbuilder = this;
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
	Assimp::Importer importer;
	importer.SetPropertyInteger( AI_CONFIG_PP_SBP_REMOVE,
		aiPrimitiveType_POINT | aiPrimitiveType_LINE );

	const struct aiScene* scene;
	scene = importer.ReadFile( _file,
		aiProcess_CalcTangentSpace		|
		aiProcess_Triangulate			|
		aiProcess_GenSmoothNormals		|	// TODO: is angle 80% already set?
		aiProcess_ValidateDataStructure	|
		aiProcess_SortByPType			|
		//aiProcess_RemoveRedundantMaterials	|
		//aiProcess_FixInfacingNormals	|
		aiProcess_FindInvalidData		|
		aiProcess_GenUVCoords			|
		aiProcess_TransformUVCoords		|
		aiProcess_ImproveCacheLocality	|
		aiProcess_JoinIdenticalVertices	|
		aiProcess_FlipUVs
		//  aiProcess_MakeLeftHanded
	);

	if(scene == nullptr) return false;

	ImportVertices( scene, scene->mRootNode, ε::identity4x4() );
	ImportMaterials( scene );

	// This is not required anymore, make place for the algorithms
	importer.FreeScene();
	return true;
}


void BVHBuilder::LoadMaterials( const std::string& _materialFileName )
{
	if( Utils::Exists(_materialFileName) )
	{
		// Simply read in the file. It is kept as it is.
		HDDFile file( _materialFileName );
		m_materials.Read( file );

		size_t n = m_materials.RootNode.Size();
		for( size_t i = 0; i < n; ++i)
		{
			const std::string& name = m_materials.RootNode[i].GetName();
			if( name.length() > 31 )
				std::cerr << "Material name to long: shortening.";
			FileDecl::Material matName;
			std::strncpy(matName.material, name.c_str(), 32);
			m_materialTable.push_back(matName);
		}

		std::cerr << "Loaded " << _materialFileName << std::endl;
	} else
		std::cerr << "No json material file yet." << std::endl;
}


void BVHBuilder::ExportGeometry( std::ofstream& _file, int _numTexcoords )
{
    // Prepare file headers and find out how much space is required
    FileDecl::NamedArray vertexHeader;
	FileDecl::NamedArray tangentsHeader;
	FileDecl::NamedArray qormalsHeader;
	FileDecl::NamedArray texcoordsHeader;
    strcpy( vertexHeader.name, "vertices" );
	strcpy( tangentsHeader.name, "tangents" );
	strcpy( qormalsHeader.name, "qormals" );
	strcpy( texcoordsHeader.name, "texcoords" );
    vertexHeader.elementSize = sizeof(FileDecl::Vertex);
	tangentsHeader.elementSize = sizeof(ε::Vec3);
	qormalsHeader.elementSize = sizeof(ε::Quaternion);
	texcoordsHeader.elementSize = sizeof(ε::Vec2);
	vertexHeader.numElements = GetVertexCount();
	tangentsHeader.numElements = GetVertexCount();
	qormalsHeader.numElements = GetVertexCount();
	texcoordsHeader.numElements = GetVertexCount();

    // Export pure vertices
    _file.write( (const char*)&vertexHeader, sizeof(FileDecl::NamedArray) );
	_file.write( (const char*)m_vertices.data(), sizeof(FileDecl::Vertex) * m_vertices.size() );

	// Export tangents
	//_file.write( (const char*)&tangentsHeader, sizeof(FileDecl::NamedArray) );
	//ExportTangents( _file, m_scene->mRootNode, ε::identity4x4() );

	// Export qormals
	//_file.write( (const char*)&qormalsHeader, sizeof(FileDecl::NamedArray) );
	//ExportQormals( _file, m_scene->mRootNode, ε::identity4x4() );

	// Export more texture coordinates
	//for(int i=1; i<_numTexcoords; ++i)
	//{
	//	_file.write( (const char*)&texcoordsHeader, sizeof(FileDecl::NamedArray) );
	//	ExportTexcoords( _file, m_scene->mRootNode, i );
	//}
}

/*void BVHBuilder::CountGeometry( const struct aiNode* _node, uint& _numVertices, uint& _numFaces )
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
}*/

void BVHBuilder::ImportVertices(
	const struct aiScene* _scene,
	const struct aiNode* _node,
	const ε::Mat4x4& _transformation)
{
	ε::Mat4x4 transformation = hard_cast<ε::Mat4x4>(_node->mTransformation) * _transformation;
	ε::Mat3x3 invTransTransform(transformation.m00, transformation.m01, transformation.m02,
								transformation.m10, transformation.m11, transformation.m12,
								transformation.m20, transformation.m21, transformation.m22);
	invTransTransform = transpose(invert(invTransTransform));
	//unsigned meshVertexOffset = (unsigned)m_vertices.size();
	for( unsigned i = 0; i < _node->mNumMeshes; ++i )
	{
		const aiMesh* mesh = _scene->mMeshes[ _node->mMeshes[i] ];

		// Find the material entry
		aiString aiName;
		_scene->mMaterials[mesh->mMaterialIndex]->Get( AI_MATKEY_NAME, aiName );
		std::string materialName = aiName.C_Str();
		uint materialIndex = 0;
		while(materialIndex < m_materials.RootNode.Size()
			&& m_materials.RootNode[materialIndex].GetName() != materialName)
			++materialIndex;
		if( materialIndex == m_materials.RootNode.Size() )
			std::cerr << "Could not find the mesh material" << std::endl;

		// Add vertices to the output array
		/*for( unsigned v = 0; v < mesh->mNumVertices; ++v )
		{
			FileDecl::Vertex vertex;
			vertex.position = ε::transform(hard_cast<ε::Vec3>(mesh->mVertices[v]), transformation);
			vertex.normal = invTransTransform * hard_cast<ε::Vec3>(mesh->mNormals[v]);
			if( mesh->HasTextureCoords(0) )
				vertex.texcoord = ε::Vec2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
			else
				vertex.texcoord = ε::Vec2(0.0f, 0.0f);
			uint32 newIdx = addVertex(vertex);
			if( newIdx != m_vertices.size()-1 )
				std::cerr << "Found redundant vertex. Bad, because index table is now wrong!" << std::endl;
		}

		// Add each triangle to the output array
		for( unsigned t = 0; t < mesh->mNumFaces; ++t )
		{
			const aiFace& face = mesh->mFaces[t];
			Assert( face.mNumIndices == 3, "This is a triangle importer!" );
			m_triangles.push_back(face.mIndices[0] + meshVertexOffset);
			m_triangles.push_back(face.mIndices[1] + meshVertexOffset);
			m_triangles.push_back(face.mIndices[2] + meshVertexOffset);
			m_triangles.push_back(materialIndex);
		}*/

		// Add each triangle to the output array and do an optional tesselation
		for( unsigned t = 0; t < mesh->mNumFaces; ++t )
		{
			const aiFace& face = mesh->mFaces[t];
			Assert( face.mNumIndices == 3, "This is a triangle importer!" );
			// Build the 3 vertices and map them to new indices
			FileDecl::Triangle triangle;
			FileDecl::Vertex vertex;
			for( unsigned j = 0; j < 3; ++j)
			{
				vertex.position = ε::transform(hard_cast<ε::Vec3>(mesh->mVertices[face.mIndices[j]]), transformation);
				vertex.normal = invTransTransform * hard_cast<ε::Vec3>(mesh->mNormals[face.mIndices[j]]);
				if( mesh->HasTextureCoords(0) )
					vertex.texcoord = ε::Vec2(mesh->mTextureCoords[0][face.mIndices[j]].x, mesh->mTextureCoords[0][face.mIndices[j]].y);
				else
					vertex.texcoord = ε::Vec2(0.0f, 0.0f);
				triangle.vertices[j] = AddVertex(vertex);
			}
			triangle.material = materialIndex;
			if( m_triangleSplitThreshold > 0.0f )
				TesselateSimple(triangle, this, m_triangleSplitThreshold);
			else TesselateNone(triangle, this);
		}

	//	meshVertexOffset += mesh->mNumVertices;
	}

	// Export all children
	for( unsigned i = 0; i < _node->mNumChildren; ++i )
		ImportVertices( _scene, _node->mChildren[i], transformation );
}

void BVHBuilder::ImportMaterials( const struct aiScene* _scene )
{
	if( !_scene->HasMaterials() )
	{
		std::cerr << "The scene has no materials to export!";
		return;
	}

	for( uint i = 0; i < _scene->mNumMaterials; ++i )
	{
		// Get name
		aiString aiName;
		auto mat = _scene->mMaterials[i];
		mat->Get( AI_MATKEY_NAME, aiName );
		std::string name = aiName.C_Str();

		// check if the material was imported before
		if( m_materials.RootNode.HasChild( name ) )
			continue;

		// The current material was not imported before do it now
		if( name.length() > 31 )
			std::cerr << "Material name to long: shortening.";
		FileDecl::Material matName;
		std::strncpy(matName.material, name.c_str(), 32);
		m_materialTable.push_back(matName);
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
}


/*void BVHBuilder::ExportTangents( std::ofstream& _file,
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
		if (mesh->mTangents != nullptr)
		{
			for (unsigned v = 0; v < mesh->mNumVertices; ++v)
			{
				ε::Vec3 tangent = invTransTransform * hard_cast<ε::Vec3>(mesh->mTangents[v]);
				_file.write((const char*)&tangent, sizeof(ε::Vec3));
			}
		}
		else
		{
			LOG_ERROR("No tangents generated! Filling tangent buffer with zeros.");
			
			ei::Vec3 zero(0, 0, 0);
			for (unsigned v = 0; v < mesh->mNumVertices; ++v)
			{
				_file.write((const char*)&zero, sizeof(ε::Vec3));
			}
		}
    }

    // Export all children
	for( unsigned i = 0; i < _node->mNumChildren; ++i )
        ExportTangents( _file, _node->mChildren[i], transformation );
}

void BVHBuilder::ExportQormals( std::ofstream& _file,
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

		if(mesh->mTangents == nullptr || mesh->mBitangents == nullptr)
		{
			LOG_ERROR("No tangent space generated! Filling qormal buffer with zeros.");
			ε::Quaternion zero = ε::qidentity();
			for (unsigned v = 0; v < mesh->mNumVertices; ++v)
			{
				_file.write((const char*)&zero, sizeof(ε::Quaternion));
			}
		} else {
			for (unsigned v = 0; v < mesh->mNumVertices; ++v)
			{
				ε::Vec3 tangent = invTransTransform * hard_cast<ε::Vec3>(mesh->mTangents[v]);
				ε::Vec3 bitangent = invTransTransform * hard_cast<ε::Vec3>(mesh->mBitangents[v]);
				ε::Vec3 normal = invTransTransform * hard_cast<ε::Vec3>(mesh->mNormals[v]);
				if(!ε::orthonormalize(normal, tangent, bitangent))
					bitangent = cross(normal, tangent);
				float handness = dot(normal, cross(tangent, bitangent));
				// If handness is wrong invert bitangent and encode the sign in the quaternion
				// which has a guaranty, that r is always positive.
				if(handness < 0.0f) bitangent = -bitangent;
				ε::Quaternion q(ε::axis(tangent, bitangent, normal));
				if(handness < 0.0f) q = -q;
				eiAssert(dot(xaxis(q), tangent) > 0.99f, "Bad qormal!");
				eiAssert(dot(yaxis(q), bitangent) > 0.99f, "Bad qormal!");
				eiAssert(dot(zaxis(q), normal) > 0.99f, "Bad qormal!");
				_file.write((const char*)&q, sizeof(ε::Quaternion));
			}
		}
    }

    // Export all children
	for( unsigned i = 0; i < _node->mNumChildren; ++i )
        ExportQormals( _file, _node->mChildren[i], transformation );
}

void BVHBuilder::ExportTexcoords( std::ofstream& _file,
        const struct aiNode* _node,
		int _texcoordChannel )
{
    for( unsigned i = 0; i < _node->mNumMeshes; ++i )
	{
		const aiMesh* mesh = m_scene->mMeshes[ _node->mMeshes[i] ];

        for( unsigned v = 0; v < mesh->mNumVertices; ++v )
        {
			ε::Vec2 texcoord;
            if( mesh->HasTextureCoords(_texcoordChannel) )
				texcoord = ε::Vec2(mesh->mTextureCoords[_texcoordChannel][v].x, mesh->mTextureCoords[_texcoordChannel][v].y);
			else
                texcoord = ε::Vec2(sqrtf(-1), sqrtf(-1));
            _file.write( (const char*)&texcoord, sizeof(ε::Vec2) );
        }
    }

    // Export all children
	for( unsigned i = 0; i < _node->mNumChildren; ++i )
        ExportTexcoords( _file, _node->mChildren[i], _texcoordChannel );
}*/


static int RecursiveTreeDepth(uint32 _idx, BVHBuilder::Node* _nodes)
{
	if(_nodes[_idx].left & 0x80000000) return 1;
	else return ε::max(RecursiveTreeDepth(_nodes[_idx].left, _nodes), RecursiveTreeDepth(_nodes[_idx].right, _nodes)) + 1;
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
	Assert( root == 0, "The root must be always the first node! Resort or allocate in perorder." );

	std::cout << "Created tree with " << m_innerNodeCount << " inner nodes and " << m_leafNodeCount << " leaves.\n";
	std::cout << "Max depth is " << RecursiveTreeDepth(0, m_nodes) << '\n';
}

void BVHBuilder::ExportApproximation( std::ofstream& _file )
{
	ComputeSGGXBases(this, m_hierarchyApproximation);

	FileDecl::NamedArray header;
	strcpy( header.name, "approx_sggx" );
	header.elementSize = sizeof(FileDecl::SGGX);
	header.numElements = m_innerNodeCount;
	_file.write( (const char*)&header, sizeof(FileDecl::NamedArray) );
	_file.write( (const char*)m_hierarchyApproximation.data(), header.numElements * header.elementSize );
}

void BVHBuilder::ExportBVH( std::ofstream& _file )
{
    // Prepare file headers and find out how much space is required
    FileDecl::NamedArray treeHeader;
    FileDecl::NamedArray bvHeader;
    strcpy( treeHeader.name, "hierarchy" );
    treeHeader.elementSize = sizeof(FileDecl::Node);
    treeHeader.numElements = m_innerNodeCount;
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
}

void BVHBuilder::ExportTriangles( std::ofstream& _file )
{
    // Prepare file headers and find out how much space is required
    FileDecl::NamedArray indexHeader;
    strcpy( indexHeader.name, "triangles" );
    indexHeader.elementSize = sizeof(FileDecl::Triangle) * FileDecl::Leaf::NUM_PRIMITIVES;
    indexHeader.numElements = m_leafNodeCount;

    // Write a "resorted index buffer" to file
    _file.write( (const char*)&indexHeader, sizeof(FileDecl::NamedArray) );
    _file.write( (const char*)m_leaves, indexHeader.elementSize * indexHeader.numElements );
}



void BVHBuilder::ExportMaterials( std::ofstream& _file, const std::string& _materialFileName )
{
	// Create the json file
	m_materials.Write( HDDFile(_materialFileName, HDDFile::OVERWRITE), Format::JSON );

	// Create the materialref table
	FileDecl::NamedArray materialHeader;
	strcpy( materialHeader.name, "materialref" );
	materialHeader.elementSize = sizeof(FileDecl::Material);
	materialHeader.numElements = static_cast<uint32>(m_materialTable.size());
	_file.write( (const char*)&materialHeader, sizeof(FileDecl::NamedArray) );

	for( uint i = 0; i < materialHeader.numElements; ++i )
	{
		_file.write( (const char*)&m_materialTable[i], sizeof(FileDecl::Material) );
	}
}

uint32 BVHBuilder::AddVertex( const FileDecl::Vertex& _vertex )
{
	// Already existant?
	VertexHandle handle;
	m_vertices.push_back(_vertex); // Temp add
	handle.id = uint32(m_vertices.size()-1);
	auto it = m_vertexToIndex.find(handle);
	if(it != m_vertexToIndex.end()) {
		m_vertices.pop_back();
		return it->second;
	}
	// Nope - "add it now" (keep temporary)
	m_vertexToIndex.emplace( handle, handle.id );
	return handle.id;
}

void BVHBuilder::AddTriangle( const FileDecl::Triangle& _triangle )
{
	m_triangles.push_back(_triangle.vertices[0]);
	m_triangles.push_back(_triangle.vertices[1]);
	m_triangles.push_back(_triangle.vertices[2]);
	m_triangles.push_back(_triangle.material);
}


ε::Triangle BVHBuilder::GetTriangle( uint32 _index ) const
{
    return ε::Triangle( m_vertices[ m_triangles[_index * 4 + 0] ].position,
                        m_vertices[ m_triangles[_index * 4 + 1] ].position,
                        m_vertices[ m_triangles[_index * 4 + 2] ].position
        );
}

ε::Triangle BVHBuilder::GetTriangle( FileDecl::Triangle _triangle ) const
{
    return ε::Triangle( m_vertices[ _triangle.vertices[0] ].position,
						m_vertices[ _triangle.vertices[1] ].position,
						m_vertices[ _triangle.vertices[2] ].position
        );
}

ε::Vec3 BVHBuilder::GetNormal( uint32 _vertexIndex ) const
{
	return m_vertices[ _vertexIndex ].normal;
}

FileDecl::Triangle BVHBuilder::GetTriangleIdx( uint32 _index ) const
{
	Assert( _index < GetTriangleCount(), "Out-of-Bounds in triangle access!" );
    FileDecl::Triangle t;
    t.vertices[0] = m_triangles[_index * 4 + 0];
    t.vertices[1] = m_triangles[_index * 4 + 1];
    t.vertices[2] = m_triangles[_index * 4 + 2];
	t.material = m_triangles[_index * 4 + 3];
    return t;
}

const FileDecl::Vertex& BVHBuilder::GetVertex( uint32 _index ) const
{
	return m_vertices[_index];
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
}


// ************************************************************************* //
bool VertexHandle::operator == (VertexHandle _rhs) const
{
	const FileDecl::Vertex* pl = &g_bvhbuilder->GetVertex(id);
	const FileDecl::Vertex* pr = &g_bvhbuilder->GetVertex(_rhs.id);
	return approx(pl->position, pr->position)
		&& approx(pl->normal, pr->normal)
		&& approx(pl->texcoord, pr->texcoord);
}

size_t std::hash<VertexHandle>::operator()(const VertexHandle& _x) const
{
	const FileDecl::Vertex* p = &g_bvhbuilder->GetVertex(_x.id);
	return hash<ε::Vec3>()(p->position) ^ hash<ε::Vec3>()(p->normal) ^ hash<ε::Vec2>()(p->texcoord);
}
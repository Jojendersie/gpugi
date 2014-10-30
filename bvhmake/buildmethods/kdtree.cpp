#include "kdtree.hpp"
#include "../../gpugi/utilities/assert.hpp"
#include <iostream>
#include <vector>
#include <algorithm>

using namespace ε;

uint32 BuildKdtree::operator()() const
{
    std::cerr << "  Sorting leaves for kd-tree..." << std::endl;

    // Create 3 sorted arrays for the dimensions
    uint32 n = m_manager->GetTriangleCount();
    std::unique_ptr<uint32[]> sorted[3] = {
        std::unique_ptr<uint32[]>(new uint32[n]),
        std::unique_ptr<uint32[]>(new uint32[n]),
        std::unique_ptr<uint32[]>(new uint32[n])
    };

    std::unique_ptr<Vec3[]> centers(new Vec3[n]);

    // Fill the index access arrays
    Initialize(sorted, centers.get());

    std::cerr << "  Building kd-tree..." << std::endl;

    return Build(sorted, centers.get(), 0, n-1);
}

void BuildKdtree::EstimateNodeCounts( uint32& _numInnerNodes, uint32& _numLeafNodes ) const 
{
    _numInnerNodes = 4 * m_manager->GetTriangleCount() / FileDecl::Leaf::NUM_PRIMITIVES;
    _numLeafNodes = 2 * m_manager->GetTriangleCount() / FileDecl::Leaf::NUM_PRIMITIVES;
}

void BuildKdtree::Initialize( const std::unique_ptr<uint32[]>* _sorted, Vec3* _centers ) const
{
    uint32 n = m_manager->GetTriangleCount();

    // Initialize unsorted and centers
	for( uint32 i = 0; i < n; ++i )
	{
		_sorted[0][i] = i;
		_sorted[1][i] = i;
		_sorted[2][i] = i;
        Triangle t = m_manager->GetTriangle( i );
        _centers[i] = (t.v0 + t.v1 + t.v2) / 3.0f;
	}

    // Sort according to center
    std::sort( &_sorted[0][0], &_sorted[0][n],
		[_centers](const uint32 _lhs, const uint32 _rhs) { return _centers[_lhs].x < _centers[_rhs].x; }
	);
    std::sort( &_sorted[1][0], &_sorted[1][n],
		[_centers](const uint32 _lhs, const uint32 _rhs) { return _centers[_lhs].y < _centers[_rhs].y; }
	);
    std::sort( &_sorted[2][0], &_sorted[2][n],
		[_centers](const uint32 _lhs, const uint32 _rhs) { return _centers[_lhs].z < _centers[_rhs].z; }
	);
}

void BuildKdtree::Split( uint32* _list, const Vec3* _centers, uint32 _size, int _splitDim, float _splitPlane ) const
{
	// Make a temporary copy
    std::vector<uint32> tmp( _size );

	// The first half should have a size of (_size + 1) / 2 which is half of
	// the elements and in case of a odd number the additional element goes to
	// the left.
	uint32 rightOff = (_size + 1) / 2;
	uint32 l = 0, r = rightOff;
	for( uint32 i = 0; i < _size; ++i )
	{
		if( _centers[_list[i]][_splitDim] <= _splitPlane )
			tmp[l++] = _list[i];
		else tmp[r++] = _list[i];
	}
	Assert( l == rightOff, "Offset of right half was wrong!" );
	Assert( l+r-rightOff == _size, "Inconsistent split - not all elements were copied!" );
	// Copy back
	memcpy( _list, &tmp[0], _size * sizeof(uint32) );
}

uint32 BuildKdtree::Build( const std::unique_ptr<uint32[]>* _sorted, Vec3* _centers, uint32 _min, uint32 _max ) const
{
    auto fit = m_manager->GetFitMethod();

    // Create a leaf if less than NUM_PRIMITIVES elements remain.
	if( _max - _min < FileDecl::Leaf::NUM_PRIMITIVES )
	{
//		Assert( _sorted[0][_min] == _sorted[1][_min] && _sorted[0][_min] == _sorted[2][_min],
//			"All sorted array should reference the same element!" );

        // Allocate a new leaf
        uint32 leafIdx = m_manager->GetNewLeaf();
        FileDecl::Leaf& leaf = m_manager->GetLeaf( leafIdx );
        // Fill it
   //     leaf.numTriangles = _max - _min;
        FileDecl::Triangle* trianglesPtr = leaf.triangles;
        for( uint i = _min; i <= _max; ++i )
            *(trianglesPtr++) = m_manager->GetTriangleIdx( _sorted[0][i] );
        for( uint i = 0; i < FileDecl::Leaf::NUM_PRIMITIVES - (_max - _min + 1); ++i )
            *(trianglesPtr++) = FileDecl::INVALID_TRIANGLE;

        // Allocate a new node pointing to this leaf
        uint32 nodeIdx = m_manager->GetNewNode();
	    BVHBuilder::Node& node = m_manager->GetNode( nodeIdx );
        node.left = 0x80000000 | leafIdx;

        // Compute a bounding volume for the new node
        (*fit)( leafIdx, nodeIdx );

		return nodeIdx;
	}

    uint32 nodeIdx = m_manager->GetNewNode();
	BVHBuilder::Node& node = m_manager->GetNode( nodeIdx );

	// Find dimension with largest extension
    Box bb;
	bb.min = Vec3( _centers[_sorted[0][_min]].x,
				   _centers[_sorted[1][_min]].y,
				   _centers[_sorted[2][_min]].z );
	bb.max = Vec3( _centers[_sorted[0][_max]].x,
				   _centers[_sorted[1][_max]].y,
				   _centers[_sorted[2][_max]].z );
	Vec3 w = bb.max - bb.min;
	int dim = 0;
	if( w[1] > w[0] && w[1] > w[2] ) dim = 1;
	if( w[2] > w[0] && w[2] > w[1] ) dim = 2;
	int codim1 = (dim + 1) % 3;
	int codim2 = (dim + 2) % 3;

	// Split at median
	uint32 m = ( _min + _max ) / 2;
	// If there are many elements with the same coordinate change their
	// positions temporary to something greater until split is done. Then
	// reset them back to the median value.
	uint32 numChanged = 0;
    float splitPlane = _centers[_sorted[dim][m]][dim];
	while( m+numChanged < _max && _centers[_sorted[dim][m+1+numChanged]][dim] == splitPlane )
	{
		// The added amount is irrelevant because it is reset after split anyway.
        _centers[_sorted[dim][m+1+numChanged]][dim] += 1.0f;
		++numChanged;
	}
	// The split requires to reorder the two other dimension arrays
	Split( &_sorted[codim1][_min], _centers, _max - _min + 1, dim, splitPlane );
	Split( &_sorted[codim2][_min], _centers, _max - _min + 1, dim, splitPlane );
	// Reset dimension values
	for( uint32 i = m+1; i < m+1+numChanged; ++i )
		_centers[_sorted[dim][i]][dim] = splitPlane;

	node.left = Build( _sorted, _centers, _min, m );
	node.right = Build( _sorted, _centers, m+1, _max );

    (*fit)( node.left, node.right, nodeIdx );

	return nodeIdx;
}
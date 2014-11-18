#include "sweep.hpp"
#include "../../gpugi/utilities/assert.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <ei/elementarytypes.hpp>

using namespace ε;

uint32 BuildSweep::operator()() const
{
    std::cerr << "  Sorting leaves for sweep algorithm..." << std::endl;

    // Create 3 sorted arrays for the dimensions
    uint32 n = m_manager->GetTriangleCount();
    std::unique_ptr<uint32[]> sorted[3] = {
        std::unique_ptr<uint32[]>(new uint32[n]),
        std::unique_ptr<uint32[]>(new uint32[n]),
        std::unique_ptr<uint32[]>(new uint32[n])
    };

    // Fill the index access arrays
    Initialize(sorted);

    std::cerr << "  Building tree via heuristic and sweep..." << std::endl;

	return Build(sorted, 0, n-1);
}

void BuildSweep::EstimateNodeCounts( uint32& _numInnerNodes, uint32& _numLeafNodes ) const 
{
    _numInnerNodes = 4 * m_manager->GetTriangleCount() / FileDecl::Leaf::NUM_PRIMITIVES;
    _numLeafNodes = 2 * m_manager->GetTriangleCount() / FileDecl::Leaf::NUM_PRIMITIVES;
}

void BuildSweep::Initialize( const std::unique_ptr<uint32[]>* _sorted ) const
{
    uint32 n = m_manager->GetTriangleCount();

    // Initialize unsorted and centers
	for( uint32 i = 0; i < n; ++i )
	{
		_sorted[0][i] = i;
		_sorted[1][i] = i;
		_sorted[2][i] = i;
        Triangle t = m_manager->GetTriangle( i );
	}

	std::unique_ptr<Vec3[]> centers(new Vec3[n]);

    // Sort according to center
    std::sort( &_sorted[0][0], &_sorted[0][n],
		[&centers](const uint32 _lhs, const uint32 _rhs) { return centers[_lhs].x < centers[_rhs].x; }
	);
    std::sort( &_sorted[1][0], &_sorted[1][n],
		[&centers](const uint32 _lhs, const uint32 _rhs) { return centers[_lhs].y < centers[_rhs].y; }
	);
    std::sort( &_sorted[2][0], &_sorted[2][n],
		[&centers](const uint32 _lhs, const uint32 _rhs) { return centers[_lhs].z < centers[_rhs].z; }
	);
}

uint32 BuildSweep::Build( const std::unique_ptr<uint32[]>* _sorted, uint32 _min, uint32 _max ) const
{
	auto fit = m_manager->GetFitMethod();

	uint32 nodeIdx = m_manager->GetNewNode();
	BVHBuilder::Node& node = m_manager->GetNode( nodeIdx );

	// Compute current bounding volume
	ComputeBoundingVolume(_sorted, _min, _max, nodeIdx, *fit);

	// Create a leaf if less than NUM_PRIMITIVES elements remain.
	if( _max - _min < FileDecl::Leaf::NUM_PRIMITIVES )
	{
		// Allocate a new leaf
        uint32 leafIdx = m_manager->GetNewLeaf();
        FileDecl::Leaf& leaf = m_manager->GetLeaf( leafIdx );
        // Fill it
        FileDecl::Triangle* trianglesPtr = leaf.triangles;
        for( uint i = _min; i <= _max; ++i )
            *(trianglesPtr++) = m_manager->GetTriangleIdx( _sorted[0][i] );
        for( uint i = 0; i < FileDecl::Leaf::NUM_PRIMITIVES - (_max - _min + 1); ++i )
            *(trianglesPtr++) = FileDecl::INVALID_TRIANGLE;

        // This node is pointing to this leaf
        node.left = 0x80000000 | leafIdx;
		node.right = 0;
	} else {
		// Assume the last indices to be unused
		uint32 tmpIdx0 = 4 * m_manager->GetTriangleCount() / FileDecl::Leaf::NUM_PRIMITIVES - 1;
		uint32 tmpIdx1 = tmpIdx0 - 1;
		uint32 tmpIdx2 = tmpIdx0 - 2;
		// Find split axis
		std::unique_ptr<Vec2[]> heuristics(new Vec2[_max - _min]);
		float minCost = std::numeric_limits<float>::infinity();
		int splitDimension = 0;
		uint32 splitIndex = 0;
		for(int d = 0; d < 3; ++d)
		{
			// Compute bounding volumes for all splits
			// Use the current node's BV memory and one additional.
			FileDecl::Triangle t = m_manager->GetTriangleIdx( _sorted[d][_min] );
			(*fit)(&t, 1, tmpIdx0);
			t = m_manager->GetTriangleIdx( _sorted[d][_max] );
			(*fit)(&t, 1, tmpIdx1);
			for(uint32 i = _min; i < _max; ++i)
			{
				// Left
				t = m_manager->GetTriangleIdx( _sorted[d][i] );
				(*fit)(&t, 1, tmpIdx2);
				(*fit)(tmpIdx0, tmpIdx2, tmpIdx0);
				// Again for the right side
				t = m_manager->GetTriangleIdx( _sorted[d][_max-i+_min] );
				(*fit)(&t, 1, tmpIdx2);
				(*fit)(tmpIdx1, tmpIdx2, tmpIdx1);
				// The volumes are rejected in the next iteration but remember
				// the result for the split decision.
				heuristics[i-_min].x   = Costs( *fit, tmpIdx0, nodeIdx, i-_min+1, _max-i );
				heuristics[_max-i-1].y = Costs( *fit, tmpIdx1, nodeIdx, _max-i, i-_min+1 );
			}

			// Find the minimum for the current dimension
			for(uint32 i = 0; i < (_max-_min); ++i)
			{
				if(sum(heuristics[i]) < minCost)
				{
					minCost = sum(heuristics[i]);
					splitDimension = d;
					splitIndex = i + _min;
				}
			}
		}

		// Perform a split which resorts the arrays
		eiAssert(splitIndex < _max, "Unexpected split index.");
		Split(_sorted, _min, _max, splitDimension, splitIndex);

		// Go into recursion
		node.left = Build(_sorted, _min, splitIndex);
		node.right = Build(_sorted, splitIndex + 1, _max);
	}
	return nodeIdx;
}

const float COST_UNDERFUL_LEAF = 0.01f;
const float COST_UNBALANCED = 0.05f;
const float COST_TRAVERSAL = 1.0f;
float BuildSweep::Costs( const FitMethod& _fit, uint32 _target, uint32 _parent, int _num, int _numOther ) const
{
	return _fit.Surface(_target) / _fit.Surface(_parent) * COST_TRAVERSAL
		+ max(0, int(FileDecl::Leaf::NUM_PRIMITIVES) - _num) * COST_UNDERFUL_LEAF / FileDecl::Leaf::NUM_PRIMITIVES
		+ (max(_numOther / float(_num), _num / float(_numOther)) - 1.0f) * COST_UNBALANCED;
}


void BuildSweep::ComputeBoundingVolume( const std::unique_ptr<uint32[]>* _sorted, uint32 _min, uint32 _max, uint32 _target, const FitMethod& _fit ) const
{
	FileDecl::Triangle t = m_manager->GetTriangleIdx( _sorted[0][_max] );
	_fit(&t, 1, _target);
	// Assume the last index to be unused
	uint32 tmpIdx = 4 * m_manager->GetTriangleCount() / FileDecl::Leaf::NUM_PRIMITIVES - 1;
	for(uint32 i = _min; i < _max; ++i)
	{
		t = m_manager->GetTriangleIdx( _sorted[0][i] );
		_fit(&t, 1, tmpIdx);
		_fit(_target, tmpIdx, _target);
	}
}

void BuildSweep::Split( const std::unique_ptr<uint32[]>* _sorted, uint32 _min, uint32 _max, int _splitDimension, uint32 _splitIndex ) const
{
	uint32 n = _max - _min + 1;
	std::vector<uint32> tmpSorted( n );
	std::vector<bool> tmpMarker( m_manager->GetTriangleCount() );

	// To decide for an triangle index it it is in the left or the right side
	// fill a lookup table.
	for(uint32 i = _min; i <= _max; ++i)
		tmpMarker[_sorted[_splitDimension][i]] = i <= _splitIndex;

	for(int d = 1; d < 3; ++d)
	{
		int codim = (_splitDimension + d) % 3;
		// Fill in bitonic order and revert right side on copy
		uint32 l=0, r=n-1;
		for(uint32 i = _min; i <= _max; ++i)
		{
			if(tmpMarker[_sorted[codim][i]])
				tmpSorted[l++] = _sorted[codim][i];
			else tmpSorted[r--] = _sorted[codim][i];
		}

		// Copy back
		for(uint32 i = 0; i < l; ++i)
			_sorted[codim][i+_min] = tmpSorted[i];
		for(uint32 i = 0; i < (n-l); ++i)
			_sorted[codim][i+_min+l] = tmpSorted[n-1-i];
	}
}

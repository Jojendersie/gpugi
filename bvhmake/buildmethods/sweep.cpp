﻿#include "sweep.hpp"
#include "../../gpugi/utilities/assert.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <ei/elementarytypes.hpp>

using namespace ε;


const float COST_UNDERFUL_LEAF = 0.01f;
const float COST_UNBALANCED = 0.88f;
const float COST_TRAVERSAL = 1.0f;
float SurfaceAreaHeuristic( const FitMethod& _fit, uint32 _target, uint32 _parent, int _num, int _numOther )
{
	float val = _fit.Surface(_target) / _fit.Surface(_parent) * COST_TRAVERSAL
		+ max(0, int(FileDecl::Leaf::NUM_PRIMITIVES) - _num) * COST_UNDERFUL_LEAF / FileDecl::Leaf::NUM_PRIMITIVES
		+ pow(1.0f - min(_numOther / float(_num), _num / float(_numOther)), 8.0f) * COST_UNBALANCED;
	//	+ (max(_numOther / float(_num), _num / float(_numOther)) - 1.0f) * COST_UNBALANCED; // with COST_UNBALANCED == 0.05
	Assert( val >= 0.0f && val <= 3.0f, "Unexpected heuristic value." );
	return val;
}



uint32 BuildSweep::operator()() const
{
    std::cerr << "  Sorting leaves for sweep algorithm..." << std::endl;

    // Create 3 sorted arrays for the dimensions
    uint32 n = m_manager->GetTriangleCount();
	std::unique_ptr<uint32[]> sorted(new uint32[n]);

    // Fill the index access arrays
    Initialize(sorted.get());

    std::cerr << "  Building tree via heuristic and sweep..." << std::endl;

	return Build(sorted.get(), 0, n-1);
}

void BuildSweep::EstimateNodeCounts( uint32& _numInnerNodes, uint32& _numLeafNodes ) const 
{
    _numInnerNodes = 4 * m_manager->GetTriangleCount() / FileDecl::Leaf::NUM_PRIMITIVES;
    _numLeafNodes = 2 * m_manager->GetTriangleCount() / FileDecl::Leaf::NUM_PRIMITIVES;
}

// Two sources to derive the z-order comparator
// (floats - unused) http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.150.9547&rep=rep1&type=pdf
// (ints - the below one uses this int-algorithm on floats) http://delivery.acm.org/10.1145/550000/545444/p472-chan.pdf?ip=141.44.23.5&id=545444&acc=ACTIVE%20SERVICE&key=2BA2C432AB83DA15.88D216EC9FFA262E.4D4702B0C3E38B35.4D4702B0C3E38B35&CFID=730905114&CFTOKEN=27227246&__acm__=1447753526_a3d46db41abc6cfb69d7f59065a4be76

// Compare if the floor base-2 logarithm of _x0^_x1 is smaller than that of _y0^_y1.
static bool lessMSB(float _x0, float _x1, float _y0, float _y1)
{
	// All numbers must be positive! Shift of the coordinates must be made before.
	Assert(_x0 >= 0.0f && _x1 >= 0.0f && _y0 >= 0.0f && _y1 >= 0.0f, "All numbers must be positive");

	// Compute masks where all large bits cancel out and only the most significant
	// is important.
	// We require the property, that floating points are ordered in their binary
	// representation.
	uint x = *reinterpret_cast<uint*>(&_x0) ^ *reinterpret_cast<uint*>(&_x1);
	uint y = *reinterpret_cast<uint*>(&_y0) ^ *reinterpret_cast<uint*>(&_y1);
	// Alternative with rounding
//	uint x = static_cast<uint>(_x0 * 100.0f) ^ static_cast<uint>(_x1 * 100.0f);
//	uint y = static_cast<uint>(_y0 * 100.0f) ^ static_cast<uint>(_y1 * 100.0f);
	return x < y && x < (x^y);
}

static bool zordercmp(const Vec3& _a, const Vec3& _b)
{
	int d = 0;
	for(int i = 1; i < 3; ++i)
		if( lessMSB(_a[d], _b[d], _a[i], _b[i]) )
			d = i;
	return _a[d] < _b[d];
}

void BuildSweep::Initialize( uint32* _sorted ) const
{
    uint32 n = m_manager->GetTriangleCount();

	std::unique_ptr<Vec3[]> centers(new Vec3[n]);

	// Initialize unsorted and centers
	Vec3 minCenter = m_manager->GetTriangle( 0 ).v0;
	for( uint32 i = 0; i < n; ++i )
	{
		_sorted[i] = i;
		Triangle t = m_manager->GetTriangle( i );
		centers[i] = (t.v0 + t.v1 + t.v2) / 3.0f;
		minCenter = min(centers[i], minCenter);
	}

	// Move the entire scene such that it gets positive for morton ordering
	for( uint32 i = 0; i < n; ++i )
		centers[i] -= minCenter;

	// Sort with morton order
	/*std::sort( &_sorted[0], &_sorted[n],
		[&centers](const uint32 _lhs, const uint32 _rhs) { return zordercmp(centers[_lhs], centers[_rhs]); }
	);*/
}

uint32 BuildSweep::Build( const uint32* _sorted, uint32 _min, uint32 _max ) const
{
	auto fit = m_manager->GetFitMethod();

	uint32 nodeIdx = m_manager->GetNewNode();
	BVHBuilder::Node& node = m_manager->GetNode( nodeIdx );

	// Compute current bounding volume
	ComputeBoundingVolume(_sorted, _min, _max, nodeIdx, *fit);

	// Create a leaf if less than NUM_PRIMITIVES elements remain.
	Assert(_min <= _max, "Node without triangles!");
	if( _max - _min < FileDecl::Leaf::NUM_PRIMITIVES )
	{
		// Allocate a new leaf
		uint32 leafIdx = m_manager->GetNewLeaf();
		FileDecl::Leaf& leaf = m_manager->GetLeaf( leafIdx );
		// Fill it
		FileDecl::Triangle* trianglesPtr = leaf.triangles;
		for( uint i = _min; i <= _max; ++i )
			*(trianglesPtr++) = m_manager->GetTriangleIdx( _sorted[i] );
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
		// Find a split index where the sum of heuristic terms left and right is minimized
		std::unique_ptr<Vec2[]> heuristics(new Vec2[_max - _min]);
		// Compute bounding volumes for all splits
		// Use the current node's BV memory and one additional.
		FileDecl::Triangle t = m_manager->GetTriangleIdx( _sorted[_min] );
		(*fit)(&t, 1, tmpIdx0);
		t = m_manager->GetTriangleIdx( _sorted[_max] );
		(*fit)(&t, 1, tmpIdx1);
		for(uint32 i = _min; i < _max; ++i)
		{
			// Left
			t = m_manager->GetTriangleIdx( _sorted[i] );
			(*fit)(&t, 1, tmpIdx2);
			(*fit)(tmpIdx0, tmpIdx2, tmpIdx0);
			// Again for the right side
			t = m_manager->GetTriangleIdx( _sorted[_max-i+_min] );
			(*fit)(&t, 1, tmpIdx2);
			(*fit)(tmpIdx1, tmpIdx2, tmpIdx1);
			// The volumes are rejected in the next iteration but remember
			// the result for the split decision.
			heuristics[i-_min].x   = SurfaceAreaHeuristic( *fit, tmpIdx0, nodeIdx, i-_min+1, _max-i );
			heuristics[_max-i-1].y = SurfaceAreaHeuristic( *fit, tmpIdx1, nodeIdx, _max-i, i-_min+1 );
		}

		// Find the minimum in sum
		float minCost = std::numeric_limits<float>::infinity();
		uint32 splitIndex = _min;
		for(uint32 i = 1; i < (_max-_min); ++i)
		{
			if(sum(heuristics[i]) < minCost)
			{
				minCost = sum(heuristics[i]);
				splitIndex = i + _min;
			}
		}

		// Go into recursion
		Assert(splitIndex >= _min && splitIndex < _max, "Unexpected split index.");
		node.left = Build(_sorted, _min, splitIndex);
		node.right = Build(_sorted, splitIndex + 1, _max);
	}
	return nodeIdx;
}


void BuildSweep::ComputeBoundingVolume( const uint32* _sorted, uint32 _min, uint32 _max, uint32 _target, const FitMethod& _fit ) const
{
	FileDecl::Triangle t = m_manager->GetTriangleIdx( _sorted[_max] );
	_fit(&t, 1, _target);
	// Assume the last index to be unused
	uint32 tmpIdx = 4 * m_manager->GetTriangleCount() / FileDecl::Leaf::NUM_PRIMITIVES - 1;
	for(uint32 i = _min; i < _max; ++i)
	{
		t = m_manager->GetTriangleIdx( _sorted[i] );
		_fit(&t, 1, tmpIdx);
		_fit(_target, tmpIdx, _target);
	}
}

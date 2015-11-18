#include "lds.hpp"
#include "sweep.hpp"
#include "../../gpugi/utilities/assert.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <ei/vector.hpp>

using namespace ε;

uint32 BuildLDS::operator()() const
{
    std::cerr << "  Building tree via largest dimension splits..." << std::endl;

	// Initialize unsorted centers and id-list
	uint32 n = m_manager->GetTriangleCount();
	std::unique_ptr<ProjCoordinate[]> centersproj(new ProjCoordinate[n]);
	std::unique_ptr<uint32[]> ids(new uint32[n]);
	for( uint32 i = 0; i < n; ++i )
	{
		ids[i] = i;
		Triangle t = m_manager->GetTriangle( i );
		centersproj[i].pos = (t.v0 + t.v1 + t.v2) / 3.0f;
	}

	return Build(ids.get(), centersproj.get(), 0, n-1);
}

void BuildLDS::EstimateNodeCounts( uint32& _numInnerNodes, uint32& _numLeafNodes ) const 
{
	// Use the same threshold as the sweep algorithm (which is one of two possible options)
    _numInnerNodes = 4 * m_manager->GetTriangleCount() / FileDecl::Leaf::NUM_PRIMITIVES;
    _numLeafNodes = 2 * m_manager->GetTriangleCount() / FileDecl::Leaf::NUM_PRIMITIVES;
}

static int DEB = 0;

uint32 BuildLDS::Build( uint32* _ids, ProjCoordinate* _centers, uint32 _min, uint32 _max ) const
{
	auto fit = m_manager->GetFitMethod();

	uint32 nodeIdx = m_manager->GetNewNode();
	BVHBuilder::Node& node = m_manager->GetNode( nodeIdx );

	// Create a leaf if less than NUM_PRIMITIVES elements remain.
	Assert(_min <= _max, "Node without triangles!");
	if( (_max - _min) < FileDecl::Leaf::NUM_PRIMITIVES )
	{
		// Allocate a new leaf
		uint32 leafIdx = m_manager->GetNewLeaf();
		FileDecl::Leaf& leaf = m_manager->GetLeaf( leafIdx );
		// Fill it
		FileDecl::Triangle* trianglesPtr = leaf.triangles;
		for( uint i = _min; i <= _max; ++i )
			*(trianglesPtr++) = m_manager->GetTriangleIdx( _ids[i] );
		for( uint i = 0; i < FileDecl::Leaf::NUM_PRIMITIVES - (_max - _min + 1); ++i )
			*(trianglesPtr++) = FileDecl::INVALID_TRIANGLE;

		// Let new inner node pointing to this leaf
		node.left = 0x80000000 | leafIdx;
		node.right = 0;

		// Compute a bounding volume for the new node
		(*fit)( leaf.triangles, _max - _min + 1, nodeIdx );

		return nodeIdx;
	}

	// Compute a covariance matrix for the current set of center points (2 passes)
	Vec3 m = _centers[_ids[_min]].pos;
	for( uint32 i = _min+1; i <= _max; ++i )
		m += _centers[_ids[i]].pos;
	m /= _max - _min + 1;
	Mat3x3 cov(0.0f);	// Variances
	for( uint32 i = _min; i <= _max; ++i )
	{
		Vec3 e = _centers[_ids[i]].pos - m;
		cov += Mat3x3(e.x*e.x, e.x*e.y, e.x*e.z,
					  e.x*e.y, e.y*e.y, e.y*e.z,
					  e.x*e.z, e.y*e.z, e.z*e.z);
	}
	cov /= _max - _min; // div n-1 for unbiased variance

	// Get the largest eigenvalues' vector, this is the direction with the largest
	// Geometry deviation. Split in this direction.
	Mat3x3 Q;
	Vec3 λ;
	decomposeQl( cov, Q, λ, false );
	/*// Fake max-dim behavior from kdtree
	Q = identity3x3();
	Vec3 vmin = _centers[_ids[_min]].pos;
	Vec3 vmax = _centers[_ids[_min]].pos;
	for( uint32 i = _min+1; i <= _max; ++i )
	{
		vmin = min(vmin, _centers[_ids[i]].pos);
		vmax = max(vmax, _centers[_ids[i]].pos);
	}
	λ = vmax - vmin;//*/
	Vec3 splitDir;
	Assert(all(λ >= 0), "Only positive eigenvalues expected!");
	if(λ.x > λ.y && λ.x > λ.z) splitDir = transpose(Q(0));
	else if(λ.y > λ.z) splitDir = transpose(Q(1));
	else splitDir = transpose(Q(2));

	// Sort current range in the projected dimension
	for( uint32 i = _min; i <= _max; ++i )
		_centers[_ids[i]].proj = dot(_centers[_ids[i]].pos, splitDir);
	std::sort( _ids + _min, _ids + _max + 1,
		[&](const uint32 _lhs, const uint32 _rhs) { return _centers[_lhs].proj < _centers[_rhs].proj; }
	);

	// Find a split position using different strategies
	uint32 splitIndex;
#ifdef LDS_SPLITMODE_MEDIAN
	// 1. Median (kd-tree) like
	splitIndex = (_min + _max) / 2;
#endif

#ifdef LDS_SPLITMODE_SWEEP
	// 2. Sweep
	ComputeBoundingVolume(_ids, _min, _max, nodeIdx, *fit);
/*	Vec3 bbsize = bbox.max - bbox.min;
	// Fake sorting along straight dimensions
	int d = 0;
	if( bbsize.y > bbsize.z && bbsize.y > bbsize.x ) d = 1;
	else if(bbsize.z > bbsize.x) d = 2;
	std::sort( _ids + _min, _ids + _max + 1,
		[&](const uint32 _lhs, const uint32 _rhs) { return _centers[_lhs].pos[d] < _centers[_rhs].pos[d]; }
	);*/
	// Reserve some nodes for temporary work Assume the last indices to be unused
	uint32 tmpIdx0 = 4 * m_manager->GetTriangleCount() / FileDecl::Leaf::NUM_PRIMITIVES - 1;
	uint32 tmpIdx1 = tmpIdx0 - 1;
	uint32 tmpIdx2 = tmpIdx0 - 2;
	// Compute lhs/rhs bounding volumes for all splits. This is done from left
	// and right adding one triangle at a time.
	// Uses the current node's BV memory and three additional.
	std::unique_ptr<Vec2[]> heuristics(new Vec2[_max - _min]);
	FileDecl::Triangle t = m_manager->GetTriangleIdx( _ids[_min] );
	(*fit)(&t, 1, tmpIdx0);
	t = m_manager->GetTriangleIdx( _ids[_max] );
	(*fit)(&t, 1, tmpIdx1);
	for(uint32 i = _min; i < _max; ++i)
	{
		// Left
		t = m_manager->GetTriangleIdx( _ids[i] );
		(*fit)(&t, 1, tmpIdx2);
		(*fit)(tmpIdx0, tmpIdx2, tmpIdx0);
		// Again for the right side
		t = m_manager->GetTriangleIdx( _ids[_max-i+_min] );
		(*fit)(&t, 1, tmpIdx2);
		(*fit)(tmpIdx1, tmpIdx2, tmpIdx1);
		// The volumes are rejected in the next iteration but remember
		// the result for the split decision.
		heuristics[i-_min].x   = SurfaceAreaHeuristic( *fit, tmpIdx0, nodeIdx, i-_min+1, _max-i );
		heuristics[_max-i-1].y = SurfaceAreaHeuristic( *fit, tmpIdx1, nodeIdx, i-_min+1, _max-i );
	}

	// Find the minimum for the current dimension
	float minCost = std::numeric_limits<float>::infinity();
	for(uint32 i = 0; i < (_max-_min); ++i)
	{
		if(sum(heuristics[i]) < minCost)
		{
			minCost = sum(heuristics[i]);
			splitIndex = i + _min;
		}
	}
#endif

	node.left = Build( _ids, _centers, _min, splitIndex );
	node.right = Build( _ids, _centers, splitIndex+1, _max );

#ifdef LDS_SPLITMODE_MEDIAN
	(*fit)( node.left, node.right, nodeIdx );
#endif

	return nodeIdx;
}

void BuildLDS::ComputeBoundingVolume( const uint32* _ids, uint32 _min, uint32 _max, uint32 _target, const FitMethod& _fit ) const
{
	FileDecl::Triangle t = m_manager->GetTriangleIdx( _ids[_max] );
	_fit(&t, 1, _target);
	// Assume the last index to be unused
	uint32 tmpIdx = 4 * m_manager->GetTriangleCount() / FileDecl::Leaf::NUM_PRIMITIVES - 1;
	for(uint32 i = _min; i < _max; ++i)
	{
		t = m_manager->GetTriangleIdx( _ids[i] );
		_fit(&t, 1, tmpIdx);
		_fit(_target, tmpIdx, _target);
	}
}
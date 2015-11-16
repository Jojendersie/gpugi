#include "lds.hpp"
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
	/*// Fake-max-dim behavior from kdtree
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
	// 1. Median (kd-tree) like
	splitIndex = (_min + _max) / 2;

	// 2. Sweep
	// TODO

	node.left = Build( _ids, _centers, _min, splitIndex );
	node.right = Build( _ids, _centers, splitIndex+1, _max );

	(*fit)( node.left, node.right, nodeIdx );

	return nodeIdx;
}

/*const float COST_UNDERFUL_LEAF = 0.01f;
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
}*/
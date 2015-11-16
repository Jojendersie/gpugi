#include "tesselate.hpp"
#include "bvhmake.hpp"

void TesselateNone(const FileDecl::Triangle& _triangle, BVHBuilder* _manager)
{
	_manager->AddTriangle(_triangle);
}

static FileDecl::Vertex AvgVertex(const BVHBuilder* _manager, uint32 i0, uint32 i1)
{
	const FileDecl::Vertex& v0 = _manager->GetVertex(i0);
	const FileDecl::Vertex& v1 = _manager->GetVertex(i1);
	FileDecl::Vertex v;
	v.position = (v0.position + v1.position) * 0.5f;
	v.normal = normalize(v0.normal + v1.normal);
	v.texcoord = (v0.texcoord + v1.texcoord) * 0.5f;
	return v;
}

void TesselateSimple(const FileDecl::Triangle& _triangle, BVHBuilder* _manager, float _maxEdgeLen)
{
	// Get edges which violate the threshold
	ε::Triangle tr = _manager->GetTriangle(_triangle);
	float e0 = lensq(tr.v2 - tr.v1);
	float e1 = lensq(tr.v0 - tr.v2);
	float e2 = lensq(tr.v0 - tr.v1);
	float thresholdSq = _maxEdgeLen * _maxEdgeLen;
	bool s0 = e0 > thresholdSq;
	bool s1 = e1 > thresholdSq;
	bool s2 = e2 > thresholdSq;
	if(!s0 && !s1 && !s2)
	{
		// Stop recursion and add final small triangle
		_manager->AddTriangle(_triangle);
		return;
	}
	// Find out which sides to tesselate for an optimal regular refined triangle.
	// The smaller the ratio the faster a triangle counts as extreme. It must be larger than
	// sqrt(2) (otherwise the triangle gets worse) and smaller 2 (can never be reached due to
	// the triangle equation a+b>c).
	const float RATIO = 1.45f; 
	// If one side is much larger then the others split only this.
	if(e0 > e1 * RATIO && e0 > e2 * RATIO) s1 = s2 = false;
	if(e1 > e0 * RATIO && e1 > e2 * RATIO) s0 = s2 = false;
	if(e2 > e0 * RATIO && e2 > e1 * RATIO) s0 = s1 = false;
	// If one side is significantly shorter then all others don't split it
	if(e0 * 1.8f < e1 && e0 * 1.8f < e2) s0 = false;
	if(e1 * 1.8f < e0 && e1 * 1.8f < e2) s1 = false;
	if(e2 * 1.8f < e0 && e2 * 1.8f < e1) s2 = false;

	// Create new vertices
	uint32 i0 = 0, i1 = 0, i2 = 0;
	if(s0) i0 = _manager->AddVertex(AvgVertex(_manager, _triangle.vertices[1], _triangle.vertices[2]));
	if(s1) i1 = _manager->AddVertex(AvgVertex(_manager, _triangle.vertices[0], _triangle.vertices[2]));
	if(s2) i2 = _manager->AddVertex(AvgVertex(_manager, _triangle.vertices[0], _triangle.vertices[1]));

	// Split by pattern
	if(s0 && s1 && s2)
	{
		TesselateSimple(FileDecl::Triangle({_triangle.vertices[0], i2, i1, _triangle.material}), _manager, _maxEdgeLen);
		TesselateSimple(FileDecl::Triangle({_triangle.vertices[1], i0, i2, _triangle.material}), _manager, _maxEdgeLen);
		TesselateSimple(FileDecl::Triangle({_triangle.vertices[2], i1, i0, _triangle.material}), _manager, _maxEdgeLen);
		TesselateSimple(FileDecl::Triangle({i0, i1, i2, _triangle.material}), _manager, _maxEdgeLen);
	} else if(s0 && s1)
	{
		TesselateSimple(FileDecl::Triangle({_triangle.vertices[2], i1, i0, _triangle.material}), _manager, _maxEdgeLen);
		if(e0 > e1)
		{
			TesselateSimple(FileDecl::Triangle({_triangle.vertices[0], _triangle.vertices[1], i0, _triangle.material}), _manager, _maxEdgeLen);
			TesselateSimple(FileDecl::Triangle({_triangle.vertices[0], i0, i1, _triangle.material}), _manager, _maxEdgeLen);
		} else {
			TesselateSimple(FileDecl::Triangle({_triangle.vertices[0], _triangle.vertices[1], i1, _triangle.material}), _manager, _maxEdgeLen);
			TesselateSimple(FileDecl::Triangle({_triangle.vertices[1], i0, i1, _triangle.material}), _manager, _maxEdgeLen);
		}
	} else if(s0 && s2)
	{
		TesselateSimple(FileDecl::Triangle({_triangle.vertices[1], i0, i2, _triangle.material}), _manager, _maxEdgeLen);
		if(e0 > e2)
		{
			TesselateSimple(FileDecl::Triangle({_triangle.vertices[2], _triangle.vertices[0], i0, _triangle.material}), _manager, _maxEdgeLen);
			TesselateSimple(FileDecl::Triangle({_triangle.vertices[0], i2, i0, _triangle.material}), _manager, _maxEdgeLen);
		} else {
			TesselateSimple(FileDecl::Triangle({_triangle.vertices[2], _triangle.vertices[0], i2, _triangle.material}), _manager, _maxEdgeLen);
			TesselateSimple(FileDecl::Triangle({_triangle.vertices[2], i2, i0, _triangle.material}), _manager, _maxEdgeLen);
		}
	} else if(s1 && s2)
	{
		TesselateSimple(FileDecl::Triangle({_triangle.vertices[0], i2, i1, _triangle.material}), _manager, _maxEdgeLen);
		if(e1 > e2)
		{
			TesselateSimple(FileDecl::Triangle({_triangle.vertices[1], _triangle.vertices[2], i1, _triangle.material}), _manager, _maxEdgeLen);
			TesselateSimple(FileDecl::Triangle({_triangle.vertices[1], i1, i2, _triangle.material}), _manager, _maxEdgeLen);
		} else {
			TesselateSimple(FileDecl::Triangle({_triangle.vertices[1], _triangle.vertices[2], i2, _triangle.material}), _manager, _maxEdgeLen);
			TesselateSimple(FileDecl::Triangle({_triangle.vertices[2], i1, i2, _triangle.material}), _manager, _maxEdgeLen);
		}
	} else if(s0) {
		TesselateSimple(FileDecl::Triangle({_triangle.vertices[0], _triangle.vertices[1], i0, _triangle.material}), _manager, _maxEdgeLen);
		TesselateSimple(FileDecl::Triangle({_triangle.vertices[2], _triangle.vertices[0], i0, _triangle.material}), _manager, _maxEdgeLen);
	} else if(s1) {
		TesselateSimple(FileDecl::Triangle({_triangle.vertices[0], _triangle.vertices[1], i1, _triangle.material}), _manager, _maxEdgeLen);
		TesselateSimple(FileDecl::Triangle({_triangle.vertices[1], _triangle.vertices[2], i1, _triangle.material}), _manager, _maxEdgeLen);
	} else if(s2) {
		TesselateSimple(FileDecl::Triangle({_triangle.vertices[2], _triangle.vertices[0], i2, _triangle.material}), _manager, _maxEdgeLen);
		TesselateSimple(FileDecl::Triangle({_triangle.vertices[1], _triangle.vertices[2], i2, _triangle.material}), _manager, _maxEdgeLen);
	} else Assert(false, "Impossible condition - at least one side must be split.");
}
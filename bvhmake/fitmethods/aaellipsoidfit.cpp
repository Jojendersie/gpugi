#include "aaellipsoidfit.hpp"
#include "ei/3dintersection.hpp"
#include "optimize.hpp"
#include "../../gpugi/utilities/assert.hpp"

ε::Ellipsoid fitFromCenter(const ε::Vec3* _vertexList, uint32 _num, ε::Vec3 _center)
{
	// Begin with an minimal ellipsoid at the given center and enlarge it
	// successive
	ε::Ellipsoid ellipsoid( _center, ε::Vec3(0.0f) );

	for(uint32 i = 0; i < _num; ++i)
	{
		// Skip point if already contained
		if( !ε::intersects(_vertexList[i], ellipsoid) )
		{
			// Compute volume minimal radii for an ellipsoid which contains
			// the point
			// Count number of dimensions via sum(präd) (if point lies on an axis
			// the optimal ellipse or beam is smaller)
			ε::Vec3 radii = sqrt((float)sum(_vertexList[i] != 0.0f)) * abs(_vertexList[i]-_center);
			// Enlarge only, thus a once added point can never fall outside.
			// This is not necessarily optimal.
			ellipsoid.radii = max(ellipsoid.radii, radii);
		}
	}

	return ellipsoid;
}

void FitEllipsoid::operator()(uint32 _left, uint32 _right, uint32 _target) const
{
	// TESTING: use bounding boxes and fit ellipsoid around
	auto elll = m_manager->GetBoundingVolume<ε::Ellipsoid>(_left);
	auto ellr = m_manager->GetBoundingVolume<ε::Ellipsoid>(_right);
	// Reconstruct the inner boxes by dividing with sqrt(3)
	elll.radii *=  0.577350269f;
	ellr.radii *=  0.577350269f;
	ε::Box box(
		ε::min(elll.center - elll.radii, ellr.center - ellr.radii),
		ε::max(elll.center + elll.radii, ellr.center + ellr.radii)
		);
	m_manager->GetBoundingVolume<ε::Ellipsoid>(_target) = ε::Ellipsoid(box);
}

void FitEllipsoid::operator()(FileDecl::Triangle* _tringles, uint32 _num, uint32 _target) const
{
	Assert( IsTriangleValid(_tringles[0]),
		"Empty leaves not allowed." );

	// Create a list of all vertices. They may be contained twice
	std::vector<ε::Vec3> vertices;
	// And their bounding box as search region for the center
	ε::Vec3 minSearch(std::numeric_limits<float>::infinity());
	ε::Vec3 maxSearch(-std::numeric_limits<float>::infinity());

	for( uint32 i = 0; i < _num && IsTriangleValid(_tringles[i]); ++i )
	{
		ε::Triangle triangle =  m_manager->GetTriangle(_tringles[i]);
		for(int j = 0; j < 3; ++j)
		{
			vertices.push_back(triangle.v(j));
			minSearch = min(minSearch, triangle.v(j));
			maxSearch = max(maxSearch, triangle.v(j));
		}
	}

	// Find optimal center and try to fit nearly optimal ellipsoid with fitFromCenter
	ε::Vec3 pos = optimize<3>(minSearch, maxSearch, [&vertices](const ε::Vec3& _center){
		return surface(fitFromCenter(vertices.data(), (int)vertices.size(), _center));
	}, 15);

	ε::Ellipsoid e = fitFromCenter(vertices.data(), (int)vertices.size(), pos);
	ε::Ellipsoid e2 = ε::Ellipsoid(ε::Box(minSearch, maxSearch));
	float vol = volume(e);
	float vol2 = volume(e2);
	//Assert(all(e.radii > 0.0f), "Degenerated cases are bad for the iterative composition!");
	m_manager->GetBoundingVolume<ε::Ellipsoid>(_target) = e;
}

float FitEllipsoid::Surface(uint32 _index) const
{
	return ε::surface( m_manager->GetBoundingVolume<ε::Ellipsoid>(_index) );
}

float FitEllipsoid::Volume(uint32 _index) const
{
	return ε::volume( m_manager->GetBoundingVolume<ε::Ellipsoid>(_index) );
}
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
			ε::Vec3 radii = sqrt((float)sum(_vertexList[i] != 0.0f)) * _vertexList[i];
			// Enlarge only, thus a once added point can never fall outside.
			// This is not necessarily optimal.
			ellipsoid.radii = max(ellipsoid.radii, radii);
		}
	}
}

void FitEllipsoid::operator()(uint32 _left, uint32 _right, uint32 _target) const
{
	// TESTING: use bounding boxes and fit ellipsoid around
	auto& elll = m_manager->GetBoundingVolume<ε::Ellipsoid>(_left);
	auto& ellr = m_manager->GetBoundingVolume<ε::Ellipsoid>(_right);
	ε::Box box(
		ε::min(elll.center - elll.radii, ellr.center - ellr.radii),
		ε::max(elll.center + elll.radii, ellr.center + ellr.radii)
		);
	m_manager->GetBoundingVolume<ε::Ellipsoid>(_target) = ε::Ellipsoid(box);
}

void FitEllipsoid::operator()(FileDecl::Triangle* _tringles, uint32 _num, uint32 _target) const
{
	// TESTING: use a box and fit an ellipsoid around
	Assert( IsTriangleValid(_tringles[0]),
        "Empty leaves not allowed." );
    // Box from first triangle
    ε::Box box( m_manager->GetTriangle(_tringles[0]) );
    // Add triangles successive
    for( uint32 i = 1; i < _num && IsTriangleValid(_tringles[i]); ++i )
        box = ε::Box(ε::Box(m_manager->GetTriangle(_tringles[i])), box);
    m_manager->GetBoundingVolume<ε::Ellipsoid>(_target) = ε::Ellipsoid(box);

	// Find optimal center and try to fit nearly optimal ellipsoid with fitFromCenter
	ε::Vec3 minSearch(-1.0f, -1.0f, -1.0f);
	ε::Vec3 maxSearch(1.0f, 1.0f, 1.0f);
	optimize<3>(minSearch, maxSearch, [](const Vec<3>&){ return 0.0f; });
}

float FitEllipsoid::Surface(uint32 _index) const
{
    return ε::surface( m_manager->GetBoundingVolume<ε::Ellipsoid>(_index) );
}

float FitEllipsoid::Volume(uint32 _index) const
{
    return ε::volume( m_manager->GetBoundingVolume<ε::Ellipsoid>(_index) );
}
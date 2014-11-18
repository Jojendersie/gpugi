#include "aaellipsoidfit.hpp"
#include "ei/3dfunctions.hpp"
#include "optimize.hpp"
#include "../../gpugi/utilities/assert.hpp"

void FitEllipsoid::operator()(uint32 _left, uint32 _right, uint32 _target) const
{
	// Decode position and radii
	//Vec<6> minSearch(-1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f);
	//Vec<6> maxSearch(-1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f);
	//optimize<6>(minSearch, maxSearch, [](const Vec<6>&){ return 0.0f; });

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
}

float FitEllipsoid::Surface(uint32 _index) const
{
    return ε::surface( m_manager->GetBoundingVolume<ε::Ellipsoid>(_index) );
}

float FitEllipsoid::Volume(uint32 _index) const
{
    return ε::volume( m_manager->GetBoundingVolume<ε::Ellipsoid>(_index) );
}
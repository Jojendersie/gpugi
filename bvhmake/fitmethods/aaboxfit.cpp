#include "aaboxfit.hpp"
#include <ei/3dtypes.hpp>
#include "../../gpugi/utilities/assert.hpp"

void FitBox::operator()(uint32 _left, uint32 _right, uint32 _target) const
{
    m_manager->GetBoundingVolume<ε::Box>(_target) = ε::Box(
         m_manager->GetBoundingVolume<ε::Box>(_left), 
         m_manager->GetBoundingVolume<ε::Box>(_right) );
}

void FitBox::operator()(FileDecl::Triangle* _tringles, uint32 _num, uint32 _target) const
{
    Assert( IsTriangleValid(_tringles[0]),
        "Empty leaves not allowed." );
    // Box from first triangle
    ε::Box box( m_manager->GetTriangle(_tringles[0]) );
    // Add triangles successive
    for( uint32 i = 1; i < _num && IsTriangleValid(_tringles[i]); ++i )
        box = ε::Box(ε::Box(m_manager->GetTriangle(_tringles[i])), box);
	Assert(all(box.min <= box.max), "Created invalid bounding box.");
    m_manager->GetBoundingVolume<ε::Box>(_target) = box;
}

float FitBox::Surface(uint32 _index) const
{
    return ε::surface( m_manager->GetBoundingVolume<ε::Box>(_index) );
}

float FitBox::Volume(uint32 _index) const
{
    return ε::volume( m_manager->GetBoundingVolume<ε::Box>(_index) );
}
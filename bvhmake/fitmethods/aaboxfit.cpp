#include "aaboxfit.hpp"
#include "ei/3dfunctions.hpp"

void FitBox::operator()(uint32 _left, uint32 _right, uint32 _target) const
{
    m_manager->GetBoundingVolume<ε::Box>(_target) = ε::Box(
         m_manager->GetBoundingVolume<ε::Box>(_left), 
         m_manager->GetBoundingVolume<ε::Box>(_right) );
}

float FitBox::Surface(uint32 _index) const
{
    return ε::surface( m_manager->GetBoundingVolume<ε::Box>(_index) );
}

float FitBox::Volume(uint32 _index) const
{
    return ε::volume( m_manager->GetBoundingVolume<ε::Box>(_index) );
}
#include "aaboxfit.hpp"
#include "ei/3dfunctions.hpp"
#include "../../gpugi/utilities/assert.hpp"

void FitBox::operator()(uint32 _left, uint32 _right, uint32 _target) const
{
    m_manager->GetBoundingVolume<ε::Box>(_target) = ε::Box(
         m_manager->GetBoundingVolume<ε::Box>(_left), 
         m_manager->GetBoundingVolume<ε::Box>(_right) );
}

void FitBox::operator()(uint32 _leaf, uint32 _target) const
{
    FileDecl::Leaf& leaf = m_manager->GetLeaf( _leaf );
    Assert( IsTriangleValid(leaf.triangles[0]),
        "Empty leaves not allowed." );
    //Assert( leaf.numTriangles > 0, "Empty leaves not allowed." );
    // Box from first triangle
    ε::Box box( m_manager->GetTriangle(leaf.triangles[0]) );
    // Add triangles successive
    for( uint32 i = 1; i < FileDecl::Leaf::NUM_PRIMITIVES && IsTriangleValid(leaf.triangles[i]); ++i )
        box = ε::Box(ε::Box(m_manager->GetTriangle(leaf.triangles[i])), box);
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
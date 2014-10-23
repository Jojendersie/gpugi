#include "kdtree.hpp"

void BuildKdtree::operator()() const
{
}

void BuildKdtree::EstimateNodeCounts( uint32& _numInnerNodes, uint32& _numLeafNodes ) const 
{
    _numInnerNodes = m_manager->GetTriangleCount() / 8;
    _numLeafNodes = m_manager->GetTriangleCount() / 8 + 1;
}
#pragma once

#include "../bvhmake.hpp"

class BuildKdtree: public BuildMethod
{
public:
    BuildKdtree(BVHBuilder* _manager) : BuildMethod(_manager) {}

    virtual void operator()() const override;

    virtual void EstimateNodeCounts( uint32& _numInnerNodes, uint32& _numLeafNodes ) const override;
};
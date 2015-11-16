#pragma once

#include "../bvhmake.hpp"

/// \brief Build with splits along the largest dimensions
class BuildLDS: public BuildMethod
{
public:
	BuildLDS(BVHBuilder* _manager) : BuildMethod(_manager) {}

    virtual uint32 operator()() const override;

    virtual void EstimateNodeCounts( uint32& _numInnerNodes, uint32& _numLeafNodes ) const override;

private:
	// Helper coordinate which can additionally store a projection along an arbitrary direction
	struct ProjCoordinate
	{
		ε::Vec3 pos;
		float proj;
	};

    /// \brief Create new tree-nodes recursively.
    uint32 Build( uint32* _ids, ProjCoordinate* _centers, uint32 _min, uint32 _max ) const;
};
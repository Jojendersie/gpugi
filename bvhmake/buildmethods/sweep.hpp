#pragma once

#include "../bvhmake.hpp"

/// \brief Evaluate a cost function for the current node.
/// \details The heuristic for a split is the sum from left and right
///		sided node costs.
float SurfaceAreaHeuristic( const FitMethod& _fit, uint32 _target, uint32 _parent, int _num, int _numOther );


class BuildSweep: public BuildMethod
{
public:
    BuildSweep(BVHBuilder* _manager) : BuildMethod(_manager) {}

    virtual uint32 operator()() const override;

    virtual void EstimateNodeCounts( uint32& _numInnerNodes, uint32& _numLeafNodes ) const override;

private:
    /// \brief Compute triangle centers and fill the 3 arrays with sorted
    ///     indices of the triangle centers.
    void Initialize( uint32* _sorted ) const;

    /// \brief The inverse operation to a merge from merge sort.
    //void Split( uint32* _list, const ε::Vec3* _centers, uint32 _size, int _splitDim, float _splitPlane ) const;

    /// \brief Create new tree-nodes recursively.
    uint32 Build( const uint32* _sorted, uint32 _min, uint32 _max ) const;

	/// \brief Compute bounding volume for all triangles in the range.
	void ComputeBoundingVolume( const uint32* _sorted, uint32 _min, uint32 _max, uint32 _target, const FitMethod& _fit ) const;

	/// \brief Split the two perpendicular dimensions such they contain the
	///		left/right part from the target dimension in two blocks.
	//void Split( const uint32* _sorted, uint32 _min, uint32 _max, int _splitDimension, uint32 _splitIndex ) const;
};
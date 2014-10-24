#pragma once

#include "../bvhmake.hpp"

class BuildKdtree: public BuildMethod
{
public:
    BuildKdtree(BVHBuilder* _manager) : BuildMethod(_manager) {}

    virtual uint32 operator()() const override;

    virtual void EstimateNodeCounts( uint32& _numInnerNodes, uint32& _numLeafNodes ) const override;

private:
    /// \brief Compute triangle centers and fill the 3 arrays with sorted
    ///     indices of the triangle centers.
    void Initialize( const std::unique_ptr<uint32[]>* _sorted, ε::Vec3* _centers ) const;

    /// \brief The inverse operation to a merge from merge sort.
    void Split( uint32* _list, const ε::Vec3* _centers, uint32 _size, int _splitDim, float _splitPlane ) const;

    /// \brief Create new kd-tree-nodes recursively.
    /// \param [out] _isLeaf The returned index is in the leaf array.
    /// \returns The index of the new root node.
    uint32 Build( const std::unique_ptr<uint32[]>* _sorted, ε::Vec3* _centers, uint32 _min, uint32 _max ) const;
};
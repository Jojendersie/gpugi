#pragma once

#include "../bvhmake.hpp"

class FitBox: public FitMethod
{
public:
    FitBox(BVHBuilder* _manager) : FitMethod(_manager) {}

    virtual void operator()(uint32 _left, uint32 _right, uint32 _target) const override;
    virtual void operator()(uint32 _leaf, uint32 _target) const override;

    virtual BVType Type() const override
    {
        return BVType::AABOX;
    }

    virtual float Surface(uint32 _index) const override;
    virtual float Volume(uint32 _index) const override;

    virtual float GetMin(uint32 _index, int _dim) const override
    {
        return m_manager->GetBoundingVolume<ε::Box>(_index).min[_dim];
    }

    virtual float GetMax(uint32 _index, int _dim) const override
    {
        return m_manager->GetBoundingVolume<ε::Box>(_index).min[_dim];
    }
};
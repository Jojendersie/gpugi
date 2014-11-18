#pragma once

#include "../bvhmake.hpp"

class FitEllipsoid: public FitMethod
{
public:
    FitEllipsoid(BVHBuilder* _manager) : FitMethod(_manager) {}

    virtual void operator()(uint32 _left, uint32 _right, uint32 _target) const override;
    virtual void operator()(FileDecl::Triangle* _tringles, uint32 _num, uint32 _target) const override;

    virtual BVType Type() const override
    {
        return BVType::AAELLIPSOID;
    }

    virtual float Surface(uint32 _index) const override;
    virtual float Volume(uint32 _index) const override;

    virtual float GetMin(uint32 _index, int _dim) const override
    {
		auto& ellipsoid = m_manager->GetBoundingVolume<ε::Ellipsoid>(_index);
        return ellipsoid.center[_dim] - ellipsoid.radii[_dim];
    }

    virtual float GetMax(uint32 _index, int _dim) const override
    {
		auto& ellipsoid = m_manager->GetBoundingVolume<ε::Ellipsoid>(_index);
        return ellipsoid.center[_dim] + ellipsoid.radii[_dim];
    }
};
#pragma once

#include "filedef.hpp" 

/// Fallback method: Simply add the triangle without tesselation.
void TesselateNone(const FileDecl::Triangle& _triangle, class BVHBuilder* _manager);

/// Split an input triangle such that no side of the resulting triangles is
///	larger than the threshold.
/// \details This algorithm works similar to GPU with inner and outer tesselation
///		and guarantees T-junction free tesselations.
/// \param [in] _triangle The triangle to be tesselated.
/// \param [out] _manager The manager to stream output the results (uses
///		addVertex and addTriangle methods)
// TODO: addVertex which returns an index and checks for uniqueness
// TODO: addTriangle which simply adds 3 indices
void TesselateSimple(const FileDecl::Triangle& _triangle, class BVHBuilder* _manager, float _maxEdgeLen);
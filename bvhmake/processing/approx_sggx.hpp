#pragma once

#include "bvhmake.hpp"

/// \brief Compute SGGX basis approximations for all inner nodes.
/// \param [out] _output A vector with on basis per inner node. They are ordered
///		like the nodes input.
void ComputeSGGXBases(const BVHBuilder* _bvhBuilder,
	std::vector<FileDecl::SGGX>& _output);
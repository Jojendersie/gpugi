#pragma once

#include <ei/matrix.hpp>
#include <functional>

template<unsigned N> using Vec = ε::Matrix<float, N, 1>;

/// \brief General purpose optimization algorithm using swarms.
/// \param [in] _function The function to be minimized.
/// \param [in] _min The minimum coordinates of the search space.
/// \param [in] _max The maximum coordinates of the search space.
template<unsigned N>
Vec<N> optimize(Vec<N> _min, Vec<N> _max, std::function<float(Vec<N>)> _function);

/// \brief General purpose optimization algorithm using evolutionary algorithm.

#include "optimize.inl"
#pragma once

#include <cstddef>
#include <vector>
#include <goc/goc.h>

#include "nyr/vrp/types.h"
#include "nyr/solutions/route.h"
#include "nyr/math/ndcpwlf.h"
#include "nyr/vrp/instance.h"

namespace nyr {

/// Arc Ready Time Functions
using ARTFs = goc::Matrix<nyr::NDCPWLF>;

/// Create a matrix of ARTFs for the given VRP instance.
ARTFs make_artfs(const VRPInstance& instance);

/// @brief Performs tree chain composition of ARTFs
/// over the given path, following the process described in
/// Visser et al. 2020: doi = {10.1287/trsc.2019.0938}
/// NOTE: Do NOT save intermediate results, just returns
/// the final NDCPWLF.
NDCPWLF perform_tree_chain_composition(
    const VRPInstance& instance,
    const ARTFs& deltas,
    const goc::GraphPath& path
);


} // namespace nyr
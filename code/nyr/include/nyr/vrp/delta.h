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

/// @brief Performs sequential chain composition of ARTFs
/// over the given path. This is the equivalent of the Lera-Romero
/// procedure, which is not optimal.
/// NOTE: Do NOT save intermediate results, just returns
/// the final NDCPWLF.
NDCPWLF perform_sequential_chain_composition(
    const VRPInstance& instance,
    const ARTFs& deltas,
    const goc::GraphPath& path
);

/// @brief Computes the optimal departure time and duration
/// for a given \delta^{\textbf{r}} RRTF.
/// NOTE: If the path is infeasible, it returns {INFTY, INFTY}.
/// Complexity: O(p) where p is the number of breakpoints in the delta path.
std::pair<nyr::TimeUnit, nyr::TimeUnit> 
compute_optimal_departure_time_and_duration(
    const nyr::NDCPWLF& delta_path
);

/// Return a RouteDuration object that contains its own 
/// copy of the path, t0, and duration.
inline RouteDuration compute_RouteDuration(
    const nyr::NDCPWLF& delta_path,
    const goc::GraphPath& path
) {
    auto [t0, duration] = compute_optimal_departure_time_and_duration(delta_path);
    return RouteDuration(path, t0, duration);
}

/// Returns a RouteDuration provided its path.
inline RouteDuration compute_RouteDuration(
    const VRPInstance& instance,
    const ARTFs& deltas,
    const goc::GraphPath& path
) {
    NDCPWLF delta_path = perform_tree_chain_composition(instance, deltas, path);
    return compute_RouteDuration(delta_path, path);
}

/// Returns a RouteDuration provided its path.
/// Uses Lera-Romero procedure which is unoptimal.
RouteDuration compute_RouteDuration_lera(
    const VRPInstance& instance,
    const goc::GraphPath& path
);

} // namespace nyr
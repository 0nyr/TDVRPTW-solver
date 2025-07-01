#pragma once

#include <goc/goc.h>
#include <nyr/nyr.h>

namespace solver {

nyr::VRPSolutionDuration regret_insertion_duration(
    const nyr::VRPInstance& vrp,
    const nyr::ARTFs& deltas,
    const nyr::VRPSolutionDuration& initial_solution
);



} // namespace solver

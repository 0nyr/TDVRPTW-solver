#pragma once

#include <goc/goc.h>
#include "instance/vrp_instance.h"

namespace solver
{

// Greedy Makespan Heuristic 1
goc::VRPSolution greedy_makespan_heuristic_1(
    const VRPInstance& vrp
);

} // namespace solver
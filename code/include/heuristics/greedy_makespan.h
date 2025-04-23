#pragma once

#include <goc/goc.h>
#include "instance/vrp_instance.h"

namespace solver
{

// Greedy Makespan Heuristic 1 (GHM1).
goc::VRPSolution greedy_makespan_heuristic_1(
    const VRPInstance& vrp
);

goc::VRPSolution ghm1_duration(
    const VRPInstance& vrp
);

goc::VRPSolution convert_makespan_solution_to_duration(
    const goc::VRPSolution& makespan_solution,
    const VRPInstance& vrp
);

std::vector<double> compute_EAT_on_free_vertices(
    const goc::Digraph& D, 
    goc::Vertex s, 
    double t0,
    const VertexSet& free_vertices, 
    const std::function<double(goc::Vertex, goc::Vertex, double)>& tt
);

} // namespace solver
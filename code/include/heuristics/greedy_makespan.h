#pragma once

#include <goc/goc.h>
#include <nyr/nyr.h>
#include "nyr/vrp/instance.h"

namespace solver
{

// Greedy Makespan Heuristic 1 (GHM1).
nyr::VRPSolutionMakespan greedy_makespan_heuristic_1(
    const nyr::VRPInstance& vrp
);

void ghm1(
    nyr::AbstractSolutionRecord& solution_record, 
    const nyr::VRPInstance& vrp, 
    const nyr::GlobalParams& gparams
);

std::vector<double> compute_EAT_on_free_vertices(
    const goc::Digraph& D, 
    goc::Vertex s, 
    double t0,
    const nyr::VertexSet& free_vertices, 
    const std::function<double(goc::Vertex, goc::Vertex, double)>& tt
);

} // namespace solver
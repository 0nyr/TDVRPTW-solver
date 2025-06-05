#pragma once

#include <concepts>

#include <goc/goc.h>
#include <nyr/nyr.h>

namespace solver
{

std::vector<double> compute_EAT_on_free_vertices(
    const goc::Digraph& D, 
    goc::Vertex s, 
    double t0,
    const nyr::VertexSet& free_vertices, 
    const std::function<double(goc::Vertex, goc::Vertex, double)>& tt
);

nyr::VRPSolutionMakespan greedy_makespan_heuristic_1(
    const nyr::VRPInstance& vrp
);

/**
 * ### Use GMH1 heuristic.
 * 
 * All routes from GMH1 are valid, but all start at t=0, 
 * they follow the Makespan objective function.
 * Auto converts to the desired solution type.
 */
template<typename Solution>
void gmh1(
    nyr::SolutionRecord<Solution>& solution_record, 
    const nyr::VRPInstance& vrp
) {
    const nyr::VRPSolutionMakespan makespan_solution = greedy_makespan_heuristic_1(vrp);
    
    // Convert the makespan solution to the desired solution type.
    auto converted_solution = nyr::auto_convert_makespan_solution<Solution>(makespan_solution, vrp);

    // Add the converted solution to the solution record.
    solution_record.try_add(converted_solution, "GMH1");
}

/** 
 * ### TD-EAT Greedy Nearest Neighbor V2
 * 
 * Constructive heuristic: Builds k routes at the
 * same time.
 * 
 * This version of the heuristic takes a (min) number of
 * routes. For every route, it computes the EAT nearest
 * neighbors, then selects the closest neighbor of all
 * neighbors of all open routes, add it to the selected route, 
 * remove this node from the list of open nodes, and continue.
 * 
*/

} // namespace solver
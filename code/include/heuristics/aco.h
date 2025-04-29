#pragma once

#include <cstdint>
#include <nyr/nyr.h>

#include <goc/goc.h>
#include "nyr/vrp/instance.h"

namespace solver
{
// All the status of the ACO algorithm.
enum class ACOStatus
{
    Finished, // The algorithm has finished.
    Converged, // The algorithm has converged.
    NoImprovement, // The algorithm has stopped due to no improvement.
    TimeLimitReached // The algorithm has stopped due to global time limit.
};

class AntCandidates
{
public:
    nyr::VRPSolutionMakespan solution; // solution built by the ant
    uint32_t nb_visited_clients; // number of visited clients
    std::vector<goc::Vertex> candidates; // candidate vertices to visit
    nyr::VertexSet free_vertices; // free vertices to visit

    AntCandidates();

    /**
     * ### Remove candidate client vertex
     */
    inline void remove_candidate(
        size_t candidate_index,
        goc::Vertex removed_candidate
    ) {
        // Swap the removed candidate with the last candidate
        assert(candidate_index < candidates.size());
        assert(candidates[candidate_index] == removed_candidate);
        candidates[candidate_index] = candidates.back();
        candidates.pop_back(); // remove last element (start depot)
        nb_visited_clients++;
        free_vertices.set(removed_candidate, false); // mark the removed candidate as visited
    }

    /**
     * ### Initialize the ant data
     * 
     * Ramdomly select a starting vertex from the candidates.
     * Initialize the candidates vector with all clients except the 
     * start depot and the preselected client.
     */
    inline void init_candidates(const nyr::VRPInstance& vrp, bool remove_end_depot = true)
    {
        candidates = vrp.D.Vertices(); // copy all vertices
        free_vertices = nyr::VertexSet().set(); // start with all vertices as free
        
        assert(candidates[0] == vrp.o);
        assert(candidates.back() == vrp.d);

        // Remove the end depot from the candidates
        if (remove_end_depot) {
            candidates.pop_back(); 
            free_vertices.set(vrp.d, false);
        }

        // remove-swap the start depot
        candidates[0] = candidates.back();
        candidates.pop_back(); // remove last element (start depot)
        free_vertices.set(vrp.o, false); // start depot is not free
    
        nb_visited_clients = 0; // no clients visited yet
    }

    inline void open_path(const nyr::VRPInstance& vrp)
    {
        // Create new empty route starting from the start depot
        solution.routes.push_back(
            nyr::RouteMakespan(
                {vrp.o}, 
                0.0
            )
        );
    }
    
    void remove_visited_client(goc::Vertex removed_candidate);

    /**
     * ### Close last path to make it a route
     */
    inline void close_path(const nyr::VRPInstance& vrp, goc::Vertex current)
    {
        solution.routes.back().path.push_back(vrp.d);
        solution.routes.back().value = vrp.ArrivalTime(
            {current, vrp.d}, 
            solution.routes.back().value
        );
        solution.value += solution.routes.back().value;
    }
};

inline double bound_pheromone_val(
    double new_val,
    double tau_min,
    double tau_max
) {
    if (new_val < tau_min)
        return tau_min;
    else if (new_val > tau_max)
        return tau_max;
    else
        return new_val;
}

ACOStatus aco(
    nyr::AbstractSolutionRecord& solution_record, 
    const nyr::VRPInstance& vrp,
    const AntColonyParams& options
);

} // namespace solver
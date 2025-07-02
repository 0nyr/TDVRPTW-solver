#pragma once

#include <goc/goc.h>
#include <nyr/nyr.h>

namespace solver {

class VisitsTracker
{
public:
    uint32_t nb_visited_clients; // number of visited clients
    std::vector<goc::Vertex> candidates; // free candidate vertices to visit

    VisitsTracker(std::vector<goc::Vertex> all_clients):   
        nb_visited_clients(0), 
        candidates(std::move(all_clients)) {}
    
    VisitsTracker(const nyr::VRPInstance& vrp): 
        VisitsTracker(vrp.copy_clients()) {}

    /// Call this method when a candidate client is visited and need
    /// to be removed from the list of candidates.
    void remove_candidate(
        size_t candidate_index,
        goc::Vertex removed_candidate
    );
};

nyr::VRPSolutionDuration greedy_nearest_neighbor_duration(
    const nyr::VRPInstance& vrp,
    const nyr::ARTFs& deltas
);




} // namespace solver
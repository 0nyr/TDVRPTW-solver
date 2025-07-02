#pragma once

#include "heuristics/greedy_duration.h"

#include <vector>

#include <goc/goc.h>
#include <nyr/nyr.h>

namespace solver {

class RegretInsertionData {
private:
    /// @brief Add an empty route.
    /// NOTE: A route is considered empty if it only contains
    /// the origin and destination depot vertices.
    inline void add_empty_route(const nyr::VRPInstance& vrp) {
        routes.push_back({vrp.o, vrp.d}); // Add a new empty route
        route_departure_times.push_back(goc::INFTY);
        route_durations.push_back(goc::INFTY);
    }

    inline void add_route(
        std::list<goc::Vertex> route,
        nyr::TimeUnit departure_time,
        nyr::TimeUnit duration
    ) {
        routes.push_back(std::move(route));
        route_departure_times.push_back(departure_time);
        route_durations.push_back(duration);
    }

    inline void add_route(
        const nyr::VRPInstance& vrp,
        const nyr::ARTFs& deltas,
        std::list<goc::Vertex> route
    ) {
        auto [departure_time, duration] = compute_optimal_departure_time_and_duration_from_path(vrp, deltas, route);
        add_route(std::move(route), departure_time, duration);
    }

public:
    VisitsTracker visits_tracker;
    std::vector<std::list<goc::Vertex>> routes; // routes being built.
    std::vector<nyr::TimeUnit> route_departure_times; // departure times of the routes.
    std::vector<nyr::TimeUnit> route_durations; // durations of the routes.

    RegretInsertionData(
        const nyr::VRPInstance& vrp,
        const nyr::ARTFs& deltas
    );

    void visit_max_regret_client(
        const nyr::VRPInstance& vrp,
        goc::Vertex max_regret_vertex,
        size_t max_regret_vertex_index,
        size_t max_regret_route_index,
        size_t max_regret_insertion_index,
        nyr::TimeUnit max_regret_modified_route_departure_time,
        nyr::TimeUnit max_regret_modified_route_duration
    );
};

/**
 * Regret Insertion Heuristic
 * 
 * A variant of the Regret Insertion of Foisy et al. (1993). 
 * Similarly to Pan et al. (2021), we also consider for any given 
 * route the possibility to insert the considered customer at the 
 * end of the route. That way, no seed client vertices are needed, 
 * and no minimal number of routes need to be provided meaning 
 * that this heuristic variant can be used directly to build a 
 * feasible solution from scratch.
 * 
 * Regret is defined as the difference between the
 * minimum duration of the route where inserting the considered
 * customer would be best, and the duration of any other route
 * where inserting the considered customer would be worse, sorted 
 * by Duration. The total regret of a client is the sum of the 
 * regrets for all theses routes.
 * 
 * The heuristic works as follows:
 * 1. For each unvisited client, for each route, compute the minimal 
 *    duration of this route when inserting the client at the best 
 *    position.
 * 2. Compute for this client, and all the routes the sum of the regrets.
 * 3. Select the client with the maximum regret, and insert it at the
 *    best position in the route with the minimum duration.
 * 4. Repeat until all clients are visited.
 * 
 * Note that an empty route is always checked for insertion.
 */
nyr::VRPSolutionDuration regret_insertion_duration(
    const nyr::VRPInstance& vrp,
    const nyr::ARTFs& deltas
);



} // namespace solver

#pragma once

#include "heuristics/greedy_duration.h"

#include <vector>
#include <ostream>

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
        auto [departure_time, duration] = compute_optimal_departure_time_and_duration_from_path(deltas, route);
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

    void print_routes(std::ostream& os);
};

/**
 * Random Insertion Heuristic
 * 
 * A simple insertion heuristic that randomly selects the next
 * client to insert into the routes. The insertion is done
 * at the best position in the route with the minimum insertion
 * cost.
 */
nyr::VRPSolutionDuration random_insertion_duration(
    const nyr::VRPInstance& vrp,
    const nyr::ARTFs& deltas
);

/// Performs Random-Insertion multiple times and return best 
/// found solution
inline nyr::VRPSolutionDuration multi_random_insertion_duration(
    const nyr::VRPInstance& vrp,
    const nyr::ARTFs& deltas,
    size_t num_runs
) {
    nyr::VRPSolutionDuration best_solution;
    best_solution.value = goc::INFTY;

    for (size_t i = 0; i < num_runs; ++i) {
        auto solution = random_insertion_duration(vrp, deltas);
        if (solution.value < best_solution.value) {
            best_solution = solution;
        }
    }

    return best_solution;
}

/**
 * Variation of the Regret Insertion Heuristic
 * (See `regret_insertion_duration`)
 * 
 * The only difference with the original Regret Insertion
 * heuristic is how the regret sum for a client is computed.
 * Instead of computing the regret for each route except the
 * one with the minimum insertion cost, we sort these other
 * routes by increasing insertion cost and compute the 
 * regret sum with the first k routes.
 */
nyr::VRPSolutionDuration regret_k_insertion_duration(
    const nyr::VRPInstance& vrp,
    const nyr::ARTFs& deltas,
    const size_t k // Common values are rather small {1, 2, 3}
);

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
 * minimum insertion cost of the route where inserting the 
 * considered customer would be best, and the duration of any 
 * other route where inserting the considered customer would 
 * be worse, sorted by insertion cost. The total regret of a 
 * client is the sum of the regrets for all theses routes.
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
inline nyr::VRPSolutionDuration regret_insertion_duration(
    const nyr::VRPInstance& vrp,
    const nyr::ARTFs& deltas
) {
    // NOTE: k=0 means that we consider all routes.
    // This is the original Regret Insertion heuristic.
    return regret_k_insertion_duration(vrp, deltas, 0);
}


} // namespace solver

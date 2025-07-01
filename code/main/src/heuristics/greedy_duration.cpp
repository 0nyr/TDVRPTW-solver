#include "heuristics/greedy_duration.h"

#include <vector>
#include <tuple>

using namespace std;
using namespace goc;
using namespace nyr;
using namespace nlohmann;

namespace solver {

void VisitsTracker::remove_candidate(
    size_t candidate_index,
    goc::Vertex removed_candidate
) {
    #ifndef NDEBUG
    assert(candidate_index < candidates.size());
    assert(candidates[candidate_index] == removed_candidate);
    #endif
    
    // Swap the removed candidate with the last candidate
    candidates[candidate_index] = candidates.back();
    candidates.pop_back(); // remove last element (start depot)
    nb_visited_clients++;
}

namespace {

std::tuple<
    size_t, 
    goc::Vertex, 
    nyr::TimeUnit, 
    nyr::TimeUnit,
    nyr::NDCPWLF
> 
select_nearest_neighbor_duration(
    const VRPInstance& vrp,
    const nyr::ARTFs& deltas,
    const VisitsTracker& visits_tracker,
    const NDCPWLF& delta_route,
    const CapacityUnit route_capacity,
    const Vertex last_visited_vertex
) {
    TimeUnit min_duration = INFTY;
    TimeUnit min_duration_departure_time = INFTY;
    Vertex min_duration_vertex = -1; // Invalid vertex initially
    size_t min_duration_index = -1; // Index of the best candidate
    NDCPWLF min_duration_delta;
    // vector<TimeUnit> candidate_durations(visits_tracker.candidates.size(), INFTY);
    
    for (size_t i = 0; i < visits_tracker.candidates.size(); ++i) {
        Vertex v = visits_tracker.candidates[i];
        
        // Check capacity constraint.
        if (vrp.q[v] + route_capacity > vrp.Q) {
            continue; // Skip if adding this vertex exceeds capacity.
        }

        // Compute the duration of the route if we add this vertex.
        NDCPWLF next_delta = deltas[last_visited_vertex][v];
        NDCPWLF new_delta_route = next_delta.compose(delta_route);
        
        // Check if the new route is feasible.
        if (new_delta_route.empty()) continue; // Skip infeasible routes.

        // Check if the end depot is reachable from the new route
        NDCPWLF new_delta_to_depot = deltas[v][vrp.d].compose(new_delta_route);
        if (new_delta_to_depot.empty()) continue; // Skip if depot is unreachable after adding the current vertex to the route.

        auto [new_departure_time, new_duration] = 
            compute_optimal_departure_time_and_duration(new_delta_route);

        if (goc::epsilon_smaller(new_duration, min_duration)) {
            // Update the best candidate.
            min_duration = new_duration;
            min_duration_departure_time = new_departure_time;
            min_duration_vertex = v;
            min_duration_index = i; // For fast removal later
            min_duration_delta = std::move(new_delta_route);
        }
    }
    return {
        min_duration_index, 
        min_duration_vertex, 
        min_duration_departure_time, 
        min_duration,
        min_duration_delta
    };
}

} // namespace anonymouss

/**
 * Greedy Nearest Neighbor Heuristic for VRP with Duration Objective
 * 
 * This heuristic constructs a solution by iteratively selecting
 * the nearest neighbor based on the smallest duration of the 
 * route to the next vertex.
 * 
 * Detailed Steps:
 * 
 * 1. Build routes one by one:
 *      - Start at the depot.
 *      - At any step, compute the duration of the last route 
 *        plus the considered vertex
 *      - Select the next vertex with the smallest duration.
 *      - Check if the depot is still reachable, if not,
 *        don't visit the latest vertex, return to the depot
 *        and close this route. Open a new empty route if some
 *        vertices are still unvisited.
 * 2. Remove visited vertices from the graph, repeat until
 *    all vertices are visited.
 * 3. Return the routes, and the sum of the duration of each route.
 */
nyr::VRPSolutionDuration greedy_nearest_neighbor_duration(
    const nyr::VRPInstance& vrp,
    const nyr::ARTFs& deltas
) {
    VRPSolutionDuration vrp_solution;
    VisitsTracker visits_tracker(vrp);
    const size_t n = vrp.nb_clients();

    // Build routes one by one.
    while (visits_tracker.nb_visited_clients < n)
    {
        GraphPath route = GraphPath({vrp.o});
        CapacityUnit route_capacity = 0.0;
        NDCPWLF delta_route = NDCPWLF::make_identity(vrp.horizon);

        // Add the next vertex with the smallest duration.
        while (true)
        {
            Vertex last_visited_vertex = route.back();
            auto [
                next_vertex_index, 
                next_vertex, 
                departure_time, 
                duration,
                next_delta_route
            ] = select_nearest_neighbor_duration(
                vrp, deltas, visits_tracker, 
                delta_route, route_capacity, 
                last_visited_vertex
            );

            if (next_vertex == -1 || goc::epsilon_bigger_equal(duration, INFTY)) { // No more candidates available
                break; // Exit the loop if no next vertex is found.
            }
            
            // Update the route with the selected vertex.
            route.push_back(next_vertex);
            route_capacity += vrp.q[next_vertex];
            visits_tracker.remove_candidate(next_vertex_index, next_vertex);
            delta_route = std::move(next_delta_route);
        }

        if (route.size() <= 1) {
            // If no vertices were added to the route, break the loop.
            break; // No more vertices to visit, exit the loop.
        }

        // Close the route by returning to the depot.
        route.push_back(vrp.d);
        delta_route = deltas[route.back()][vrp.d].compose(delta_route);
        auto [departure_time, duration] = 
            compute_optimal_departure_time_and_duration(delta_route);
        
        #ifndef NDEBUG
        bool throw_error = false;
        if (goc::epsilon_bigger_equal(duration, INFTY)) {
            std::clog << "greedy_nearest_neighbor_duration: Error: "
                << "Route duration is INFTY.";
            throw_error = true;
        }
        if (goc::epsilon_bigger_equal(departure_time, INFTY)) {
            std::clog << "greedy_nearest_neighbor_duration: Error: "
                << "Route departure_time is INFTY.";
            throw_error = true;
        }
        if (throw_error) {
            std::clog << " current route: " << route 
                << ", departure time: " << departure_time
                << ", duration: " << duration
                << ", nb of already built routes: " << vrp_solution.routes.size()
                << ", nb visited clients: " << visits_tracker.nb_visited_clients
                << ", nb remaining candidates: " << visits_tracker.candidates.size()
                << ", candidates: " << visits_tracker.candidates
                << std::endl;
            throw std::runtime_error("greedy_nearest_neighbor_duration: Error: "
                "Route is infeasible when closing it to end depot. Should not happen.");
        }
        #endif

        // Create a new route with the computed values.
        RouteDuration route_duration(
            std::move(route), 
            departure_time, 
            duration
        );
        vrp_solution.routes.push_back(std::move(route_duration));
    }

    return vrp_solution;
}


} // namespace solver
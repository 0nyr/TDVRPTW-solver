#include "heuristics/regret_insertion.h"
#include "heuristics/greedy_duration.h"

#include <vector>

using namespace std;
using namespace goc;
using namespace nyr;
using namespace nlohmann;

namespace solver {

nyr::VRPSolutionDuration regret_insertion_duration(
    const nyr::VRPInstance& vrp,
    const nyr::ARTFs& deltas
) {
    VRPSolutionDuration vrp_solution;
    VisitsTracker visits_tracker(vrp);
    auto [first_client, d] = vrp.clients_range();
    const size_t n = vrp.nb_clients();
    
    vector<list<Vertex>> routes; // routes being built.
    routes.push_back({vrp.o}); // Init with first route starting at depot.

    // Insert the client with max regret until all clients are visited.
    while (visits_tracker.nb_visited_clients < n)
    {
        // // WARN: Due to clients starting at index 1, we consider a larger vector of regrets to avoid index magic.
        // vector<TimeUnit> vertex_regrets(vrp.nb_vertices(), 0);
        TimeUnit max_regret = 0;
        Vertex max_regret_vertex = -1;
        size_t max_regret_vertex_index = -1; // Index of the vertex with max regret in the list of candidates.
        size_t max_regret_route_index = -1;
        size_t max_regret_insertion_index = -1;

        // Compute the regret for each unvisited client.
        // And select the one with the maximum regret.
        for (size_t i = 0; i < visits_tracker.candidates.size(); ++i) {
            Vertex v = visits_tracker.candidates[i];

            // For each unvisited client, compute the regret for each route.
            size_t best_insertion_index = -1;
            size_t best_route_index = -1;
            TimeUnit best_duration = INFTY;

            vector<TimeUnit> route_best_durations(routes.size(), INFTY);

            for (size_t route_index = 0; route_index < routes.size(); ++route_index) {
                // We use routes as list of vertices to allow fast insertion and removal.
                auto& route = routes[route_index];
                
                // Compute the best insertion position for this client in the route.
                TimeUnit min_duration_with_insertion = INFTY;
                TimeUnit associated_departure_time = INFTY;
                size_t best_insertion_pos = -1;

                // WARN: Cannot insert before the first element (start depot).
                for (size_t insertion_index = 1; insertion_index <= route.size(); ++insertion_index) {
                    // Insert the client at the current position.
                    route.insert(std::next(route.begin(), insertion_index), v);
                    
                    // Compute the duration of the new route where the client is inserted.
                    NDCPWLF delta_new_route = perform_tree_chain_composition(
                        vrp, deltas, route
                    );
                    auto [
                        new_departure_time, new_duration
                    ] = compute_optimal_departure_time_and_duration(
                        delta_new_route
                    );

                    if (new_duration < min_duration_with_insertion) {
                        min_duration_with_insertion = new_duration;
                        best_insertion_pos = insertion_index;
                        associated_departure_time = new_departure_time;
                    }

                    // Restablish the original route.
                    route.erase(std::next(route.begin(), insertion_index));
                }

                route_best_durations[route_index] = min_duration_with_insertion;
                if (min_duration_with_insertion < best_duration) {
                    best_duration = min_duration_with_insertion;
                    best_insertion_index = best_insertion_pos;
                    best_route_index = route_index;
                }
            }

            // Compute the regrets and sum of regrets.
            TimeUnit sum_of_regrets = 0;
            for (size_t route_index = 0; route_index < routes.size(); ++route_index) {
                if (route_index == best_route_index) {
                    continue; // Skip the best route, since its regret is 0.
                }

                TimeUnit regret = route_best_durations[route_index] - best_duration;
                #ifndef NDEBUG
                if (regret < 0) {
                    throw std::logic_error("Regret cannot be negative.");
                }
                #endif
                sum_of_regrets += regret;
            }

            //vertex_regrets[v] = sum_of_regrets;
            if (sum_of_regrets > max_regret) {
                max_regret = sum_of_regrets;
                max_regret_vertex = v;
                max_regret_vertex_index = i;
                max_regret_route_index = best_route_index;
                max_regret_insertion_index = best_insertion_index;
            }
        }

        // The max regret vertex is the one to insert.
        #ifndef NDEBUG
        if (max_regret_vertex == -1) {
            throw std::logic_error("No vertex with max regret found.");
        }
        #endif

        // Insert the max regret vertex in the best route at the best position.
        auto& best_route = routes[max_regret_route_index];
        // WARN: If this was a route with no clients, add a new empty route.
        if (best_route.size() == 1) {
            routes.push_back({vrp.o}); // Add a new route starting at depot.
        }
        best_route.insert(
            std::next(best_route.begin(), max_regret_insertion_index), 
            max_regret_vertex
        );
        visits_tracker.remove_candidate(
            max_regret_vertex_index, max_regret_vertex
        );
    }

    // Build final solution from the routes.
    TimeUnit duration_sum = 0;
    for (const auto& route : routes) {
        if (route.size() <= 1) {
            continue; // Skip empty routes (only depot).
        }
        // Convert the route to a GraphPath.
        GraphPath solution_route(route.begin(), route.end());
        // Close the solution_route
        solution_route.push_back(vrp.d); // Add depot at the end.
        vrp_solution.routes.push_back(
            compute_RouteDuration(
                vrp, deltas, solution_route
            )
        );
        duration_sum += vrp_solution.routes.back().value;
    }
    vrp_solution.value = duration_sum;

    return vrp_solution;
}

} // namespace solver

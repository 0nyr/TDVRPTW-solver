#include "heuristics/regret_insertion.h"
#include "heuristics/greedy_duration.h"

using namespace std;
using namespace goc;
using namespace nyr;
using namespace nlohmann;

namespace solver {

RegretInsertionData::RegretInsertionData(
    const nyr::VRPInstance& vrp,
    const nyr::ARTFs& deltas
):
    visits_tracker(vrp), routes(), route_departure_times(), route_durations()
{
    // Initialize the first route.
    // WARN: For the algorithm to work without edge cases, we need to initialize
    // the routes with one non-empty client route and one empty client route.
    // This is because the algorithm computes the regret based on at least 2 routes 
    // otherwise the regret is always 0 (since the regret is computed as the difference
    // between the best route insertion cost and another route insertion cost).
    add_route(
        vrp, deltas,
        {vrp.o, visits_tracker.candidates.front(), vrp.d}
    );
    // GraphPath initial_route(routes[0].begin(), routes[0].end());
    visits_tracker.remove_candidate(
        0,
        visits_tracker.candidates.front()
    );
    add_empty_route(vrp); // We need to maintain a single empty route
}

void RegretInsertionData::visit_max_regret_client(
    const nyr::VRPInstance& vrp,
    goc::Vertex max_regret_vertex,
    size_t max_regret_vertex_index,
    size_t max_regret_route_index,
    size_t max_regret_insertion_index,
    TimeUnit max_regret_modified_route_departure_time,
    TimeUnit max_regret_modified_route_duration
) {
    // Insert the max regret vertex in the best route at the best position.
    auto& best_route = routes[max_regret_route_index];
    best_route.insert(
        std::next(best_route.begin(), max_regret_insertion_index), 
        max_regret_vertex
    );
    visits_tracker.remove_candidate(
        max_regret_vertex_index, max_regret_vertex
    );
    route_departure_times[max_regret_route_index] = 
        max_regret_modified_route_departure_time;
    route_durations[max_regret_route_index] = 
        max_regret_modified_route_duration;

    // WARN: If this was a route with no clients, add a new empty route.
    if (best_route.size() == 3) {
        add_empty_route(vrp);

        #ifndef NDEBUG
        // Assert that there is only one empty route.
        if (std::count_if(routes.begin(), routes.end(), [](const auto& r) {
            return r.size() == 2; // Only depot.
        }) > 1) {
            std::clog << "Error: More than one empty route found." << std::endl;
            for (size_t i = 0; i < routes.size(); ++i) {
                const auto& route = routes[i];
                std::clog << " - Route[" << i << "]: ";
                if (route.empty()) {
                    std::clog << "empty";
                } else {
                    for (const auto& v : route) {
                        std::clog << v << " ";
                    }
                }
                std::clog << " (size: " << route.size() << ")";
                std::clog << "\n";
            }
            throw std::logic_error("More than one empty route found.");
        }
        #endif
    }
}

void RegretInsertionData::print_routes(std::ostream& os) {
    for (size_t i = 0; i < routes.size(); ++i) {
        os << "Route " << i << ": ";
        for (const auto& v : routes[i]) {
            os << v << " ";
        }
        os << "(departure: " << route_departure_times[i]
           << ", duration: " << route_durations[i] << ")";
        os << std::endl;
    }
}

std::tuple<TimeUnit, TimeUnit, TimeUnit, size_t>
compute_best_insertion_position(
    const nyr::ARTFs& deltas,
    std::list<goc::Vertex>& route,
    const TimeUnit route_duration,
    goc::Vertex v
) {
    // Compute the best insertion position for this client in the route.
    TimeUnit min_insertion_cost = INFTY;
    TimeUnit associated_departure_time = INFTY;
    TimeUnit associated_duration = INFTY;
    size_t best_insertion_pos = -1;

    // WARN: Cannot insert before the first element (start depot).
    // WARN: Cannot insert after the last element (end depot).
    for (size_t insertion_index = 1; insertion_index < route.size(); ++insertion_index) {
        // Insert the client at the current position.
        route.insert(std::next(route.begin(), insertion_index), v);
        
        // Compute the duration of the new route where the client is inserted.
        NDCPWLF delta_new_route = perform_tree_chain_composition(
            deltas, route // modified with v inserted
        );
        auto [
            new_departure_time, new_duration
        ] = compute_optimal_departure_time_and_duration(
            delta_new_route
        );

        TimeUnit insertion_cost = new_duration - route_duration;
        if (insertion_cost < min_insertion_cost) {
            min_insertion_cost = insertion_cost;
            associated_departure_time = new_departure_time;
            associated_duration = new_duration;
            best_insertion_pos = insertion_index;
        }

        // Restablish the original route.
        route.erase(std::next(route.begin(), insertion_index));
    }

    return std::make_tuple(
        min_insertion_cost, 
        associated_departure_time, 
        associated_duration, 
        best_insertion_pos
    );
}


nyr::VRPSolutionDuration random_insertion_duration(
    const nyr::VRPInstance& vrp,
    const nyr::ARTFs& deltas
) {
    // Random insertion heuristic.
    VRPSolutionDuration vrp_solution;
    RegretInsertionData data(vrp, deltas);
    const size_t n = vrp.nb_clients();

    // Insert the client with max regret until all clients are visited.
    while (data.visits_tracker.nb_visited_clients < n)
    {
        #ifndef NDEBUG
        std::clog << "  [iter: " << data.visits_tracker.nb_visited_clients
            << "/" << n << "]"
            << " nb routes: " << data.routes.size()
            << "\n";
        // data.print_routes(std::clog);
        #endif
        
        // Select a random unvisited client.
        size_t random_index = rand_index(data.visits_tracker.candidates.size());
        goc::Vertex v = data.visits_tracker.candidates[random_index];

        TimeUnit best_insertion_cost = INFTY;
        size_t best_insertion_index = -1;
        size_t best_route_index = -1;
        TimeUnit best_duration = INFTY;
        TimeUnit best_departure_time = INFTY;

        for (size_t route_index = 0; route_index < data.routes.size(); ++route_index) {
            // We use routes as list of vertices to allow fast insertion and removal.
            auto& route = data.routes[route_index];
            TimeUnit route_duration = data.route_durations[route_index];
            if (route_duration >= INFTY) {
                #ifndef NDEBUG
                // Check that it is an empty route
                if (route.size() != 2 || route.front() != vrp.o || route.back() != vrp.d) {
                    std::clog << "Error: Route duration is INFTY but the route is not empty." << std::endl;
                    throw std::logic_error("Route duration cannot be INFTY for non-empty routes.");
                }
                #endif
                // For empty route, the insertion cost is the duration of the route,
                // so we fix route_duration to 0.
                // WARN: This is a special case for empty routes.
                // TODO: Modify the insertion of empty routes to directly have a duration of 0
                route_duration = 0;
            }

            // Compute the best insertion position for this client in the route.
            auto [
                min_insertion_cost, 
                associated_departure_time, 
                associated_duration, 
                best_insertion_pos
            ] = compute_best_insertion_position(
                deltas, 
                route, 
                route_duration,
                v
            );

            #ifndef NDEBUG
            std::clog
                << "    r°" << route_index
                << " min insert cost: " << min_insertion_cost
                << std::endl;
            #endif

            if (min_insertion_cost < best_insertion_cost) {
                best_insertion_cost = min_insertion_cost;
                best_insertion_index = best_insertion_pos;
                best_route_index = route_index;
                best_duration = associated_duration;
                best_departure_time = associated_departure_time;
            }
        }

        #ifndef NDEBUG
        std::clog 
            << "Visiting client: " << v 
            << " with min insertion cost: " << best_insertion_cost
            << " in route n°" << best_route_index
            << ", at pos: " << best_insertion_index
            << ", dep time: " << best_departure_time
            << ", duration: " << best_duration
            << "\n";
        #endif

        // Visit the randomly selected client at its best insertion location
        data.visit_max_regret_client(
            vrp, v, random_index,
            best_route_index,
            best_insertion_index,
            best_departure_time,
            best_duration
        );
    }

    // Build final solution from the routes.
    // TODO: Make standalone function of the following:
    TimeUnit duration_sum = 0;
    for (size_t route_index = 0; route_index < data.routes.size(); ++route_index) {
        const auto& route = data.routes[route_index];
        if (route.size() <= 2) {
            continue; // Skip empty routes (no visited clients).
        }
        // Convert the route to a GraphPath.
        GraphPath solution_route(route.begin(), route.end());
        TimeUnit route_duration = data.route_durations[route_index];
        TimeUnit route_departure_time = data.route_departure_times[route_index];
        
        #ifndef NDEBUG
        // NOTE: his is a debug check to ensure correct duration of route.
        auto [recomputed_departure_time, recomputed_duration] = 
            compute_optimal_departure_time_and_duration_from_path(
                deltas, solution_route
            );
        if (recomputed_departure_time != route_departure_time ||
            recomputed_duration != route_duration) {
            std::clog << "Error: Recomputed departure time or duration does not match the stored values.\n"
                      << " - Expected: (" << route_departure_time << ", " << route_duration << ")\n"
                      << " - Got: (" << recomputed_departure_time << ", " << recomputed_duration << ")\n";
            throw std::logic_error("Recomputed departure time or duration does not match the stored values.");
        }
        #endif

        vrp_solution.routes.push_back(
            // compute_RouteDuration(
            //     vrp, deltas, solution_route
            // )
            RouteDuration(
                solution_route, route_departure_time, route_duration
            )
        );
        duration_sum += vrp_solution.routes.back().value;
    }
    vrp_solution.value = duration_sum;

    return vrp_solution;
}

nyr::VRPSolutionDuration regret_k_insertion_duration(
    const nyr::VRPInstance& vrp,
    const nyr::ARTFs& deltas,
    const size_t k
) {
    VRPSolutionDuration vrp_solution;
    RegretInsertionData data(vrp, deltas);
    const size_t n = vrp.nb_clients();
    
    // Insert the client with max regret until all clients are visited.
    while (data.visits_tracker.nb_visited_clients < n)
    {
        // // WARN: Due to clients starting at index 1, we consider a larger vector of regrets to avoid index magic.
        // vector<TimeUnit> vertex_regrets(vrp.nb_vertices(), 0);
        TimeUnit max_regret = -INFTY;
        Vertex max_regret_vertex = -1;
        size_t max_regret_vertex_index = -1; // Index of the vertex with max regret in the list of candidates.
        size_t max_regret_route_index = -1;
        size_t max_regret_insertion_index = -1;
        TimeUnit max_regret_modified_route_departure_time = INFTY;
        TimeUnit max_regret_modified_route_duration = INFTY;

        #ifndef NDEBUG
        std::clog << "  [iter: " << data.visits_tracker.nb_visited_clients
            << "/" << n << "]"
            << " nb routes: " << data.routes.size()
            << "\n";
        #endif

        // Compute the regret for each unvisited client.
        // And select the one with the maximum regret.
        for (size_t i = 0; i < data.visits_tracker.candidates.size(); ++i) {
            Vertex v = data.visits_tracker.candidates[i];

            // For each unvisited client, compute the regret for each route.
            TimeUnit best_insertion_cost = INFTY;
            size_t best_insertion_index = -1;
            size_t best_route_index = -1;
            TimeUnit best_duration = INFTY;
            TimeUnit best_departure_time = INFTY;

            // We need to store all insertion costs to be able to determine the min.
            vector<TimeUnit> route_min_insertion_costs(data.routes.size(), INFTY);
            vector<size_t> route_indices; // Used when k > 0 to store the sorted indices of the routes.

            for (size_t route_index = 0; route_index < data.routes.size(); ++route_index) {
                // We use routes as list of vertices to allow fast insertion and removal.
                auto& route = data.routes[route_index];
                TimeUnit route_duration = data.route_durations[route_index];
                if (route_duration >= INFTY) {
                    #ifndef NDEBUG
                    // Check that it is an empty route
                    if (route.size() != 2 || route.front() != vrp.o || route.back() != vrp.d) {
                        std::clog << "Error: Route duration is INFTY but the route is not empty." << std::endl;
                        throw std::logic_error("Route duration cannot be INFTY for non-empty routes.");
                    }
                    #endif
                    // For empty route, the insertion cost is the duration of the route,
                    // so we fix route_duration to 0.
                    // WARN: This is a special case for empty routes.
                    // TODO: Modify the insertion of empty routes to directly have a duration of 0
                    route_duration = 0;
                }
                
                // Compute the best insertion position for this client in the route.
                TimeUnit min_insertion_cost = INFTY;
                TimeUnit associated_departure_time = INFTY;
                TimeUnit associated_duration = INFTY;
                size_t best_insertion_pos = -1;

                // WARN: Cannot insert before the first element (start depot).
                // WARN: Cannot insert after the last element (end depot).
                for (size_t insertion_index = 1; insertion_index < route.size(); ++insertion_index) {
                    // Insert the client at the current position.
                    route.insert(std::next(route.begin(), insertion_index), v);
                    
                    // Compute the duration of the new route where the client is inserted.
                    NDCPWLF delta_new_route = perform_tree_chain_composition(
                        deltas, route // modified with v inserted
                    );
                    auto [
                        new_departure_time, new_duration
                    ] = compute_optimal_departure_time_and_duration(
                        delta_new_route
                    );

                    TimeUnit insertion_cost = new_duration - route_duration;
                    if (insertion_cost < min_insertion_cost) {
                        min_insertion_cost = insertion_cost;
                        associated_departure_time = new_departure_time;
                        associated_duration = new_duration;
                        best_insertion_pos = insertion_index;
                    }

                    // Restablish the original route.
                    route.erase(std::next(route.begin(), insertion_index));
                }

                route_min_insertion_costs[route_index] = min_insertion_cost;
                if (min_insertion_cost < best_insertion_cost) {
                    best_insertion_cost = min_insertion_cost;
                    best_insertion_index = best_insertion_pos;
                    best_route_index = route_index;
                    best_duration = associated_duration;
                    best_departure_time = associated_departure_time;
                }
            }

            if (k > 0) {
                // We need to consider only the k best routes.
                // Sort the route_min_insertion_costs to find the k best routes.
                route_indices = nyr::sort_indices_by_ascending_values(route_min_insertion_costs);
            }

            // Compute the regrets and sum of regrets.
            TimeUnit sum_of_regrets = 0;
            for (size_t j = 0; j < data.routes.size(); ++j) {
                size_t route_index = j;
                if (k > 0) {
                    // If k > 0, we consider only the k best routes.
                    if (j >= k) {
                        break; // Stop after k best routes.
                    }
                    route_index = route_indices[j]; // Get the index of the j-th best route.
                }

                if (route_index == best_route_index) {
                    continue; // Skip the best route, since its regret is 0.
                }

                TimeUnit regret = route_min_insertion_costs[route_index] - best_insertion_cost;
                // #ifndef NDEBUG
                if (regret < 0) {
                    throw std::logic_error("Regret cannot be negative.");
                }
                // #endif
                sum_of_regrets += regret;
            }

            #ifndef NDEBUG
            // Print debug information.
            std::clog 
                << "    + v: " << v
                << ", sum-regret: " << sum_of_regrets
                << std::endl;
            #endif

            //vertex_regrets[v] = sum_of_regrets;
            if (sum_of_regrets > max_regret) {
                max_regret = sum_of_regrets;
                max_regret_vertex = v;
                max_regret_vertex_index = i;
                max_regret_route_index = best_route_index;
                max_regret_insertion_index = best_insertion_index;
                max_regret_modified_route_departure_time = best_departure_time;
                max_regret_modified_route_duration = best_duration;
            }
        }

        // The max regret vertex is the one to insert.
        data.visit_max_regret_client(
            vrp,
            max_regret_vertex,
            max_regret_vertex_index,
            max_regret_route_index,
            max_regret_insertion_index,
            max_regret_modified_route_departure_time,
            max_regret_modified_route_duration
        );
    }

    // Build final solution from the routes.
    TimeUnit duration_sum = 0;
    for (size_t route_index = 0; route_index < data.routes.size(); ++route_index) {
        const auto& route = data.routes[route_index];
        if (route.size() <= 2) {
            continue; // Skip empty routes (no visited clients).
        }
        // Convert the route to a GraphPath.
        GraphPath solution_route(route.begin(), route.end());
        TimeUnit route_duration = data.route_durations[route_index];
        TimeUnit route_departure_time = data.route_departure_times[route_index];
        
        #ifndef NDEBUG
        // NOTE: his is a debug check to ensure correct duration of route.
        auto [recomputed_departure_time, recomputed_duration] = 
            compute_optimal_departure_time_and_duration_from_path(
                deltas, solution_route
            );
        if (recomputed_departure_time != route_departure_time ||
            recomputed_duration != route_duration) {
            std::clog << "Error: Recomputed departure time or duration does not match the stored values.\n"
                      << " - Expected: (" << route_departure_time << ", " << route_duration << ")\n"
                      << " - Got: (" << recomputed_departure_time << ", " << recomputed_duration << ")\n";
            throw std::logic_error("Recomputed departure time or duration does not match the stored values.");
        }
        #endif

        vrp_solution.routes.push_back(
            // compute_RouteDuration(
            //     vrp, deltas, solution_route
            // )
            RouteDuration(
                solution_route, route_departure_time, route_duration
            )
        );
        duration_sum += vrp_solution.routes.back().value;
    }
    vrp_solution.value = duration_sum;

    return vrp_solution;
}

} // namespace solver

#include "heuristics/greedy_makespan.h"
#include "instance/vrp_instance.h"

#include <vector>
#include <tuple>
#include <optional>

using namespace std;
using namespace goc;
using namespace nlohmann;

namespace solver
{

/**
 * Compute the earliest arrival time from a given vertex
 * at a given departure time, while considering 
 * only a subset of the vertices (free vertices).
 */
vector<double> compute_EAT_on_free_vertices(
    const Digraph& D, 
    Vertex s, 
    double t0,
    const VertexSet& free_vertices, 
    const function<double(Vertex, Vertex, double)>& tt
) {
    priority_queue<pair<double, Vertex>, vector<pair<double, Vertex>>, greater<>> q;
    vector<bool> visited(D.NbVertices(), false);
    vector<double> EAT(D.NbVertices(), INFTY); // EAT[j] = Earliest arrival time to vertex j
    q.push({t0, s});
    while (!q.empty())
    {
        double t; Vertex v;
        tie(t, v) = q.top();
        q.pop();
        if (visited[v]) continue;
        visited[v] = true;
        EAT[v] = t;
        for (auto& w: D.Successors(v))
        {
            if (!visited[w] && contains(free_vertices, w))
            {
                double travel_time = tt(v, w, t);
                if (travel_time == INFTY) continue;
                q.push({t + travel_time, w}); // t + travel time == arrival time
            }
        }
    }

    return EAT;
}




/**
 * ### Greedy Makespan Heuristic 1
 * 
 * Pure Makespan mode: each route starts at t=0.
 * Waiting is only useful to wait for TW ealiest arrivals
 * due to the FIFO property. 
 * 
 * Greedy Makespan Heuristic 1:
 * 1. Build routes one by one:
 *  - Start at the depot.
 *  - Add the next vertex with the smallest makespan (earliest arrival time).
 *  - After each addition, check if the depot is still reachable, 
 *    if not, don't visit the latest vertex, return to the depot
 *    and close this route.
 * 2. Remove visited vertices from the graph, repeat until
 *    all vertices are visited.
 * 3. Return the routes, and the sum of the makespan of each route.
 */
VRPSolution greedy_makespan_heuristic_1(
    const VRPInstance& vrp
) {
    // Step 1: Initialize the solution.
    vector<Route> routes;
    double total_makespan = 0.0;
    VertexSet visited_vertices;
    const size_t n = vrp.D.NbVertices();
    const auto& V = vrp.D.Vertices();

    clog << "Horizon: [0, " << vrp.T << "]" << endl;
    clog << "Depot (start & end): " << vrp.o << " - " << vrp.d << endl;
    clog << "Max capacity Q: " << vrp.Q << endl;

    // Step 2: Build routes one by one.
    while (nb_bits_set(visited_vertices) + 2 < n)
    {
        // Step 2.1: Start at the depot.
        Route route = Route({vrp.o}, 0.0, 0.0);
        CapacityUnit route_capacity = 0.0;

        // Step 2.2: Add the next vertex with the smallest makespan.
        while (true)
        {
            // Find the next vertex to visit.
            Vertex current_vertex = route.path.back();
            VertexSet free_vertices = difference(
                VertexSet().set(), visited_vertices
            );
            //clog << "GMH1: Free vertices: " << free_vertices << endl;
            vector<double> makespans_i_t = compute_EAT_on_free_vertices(
                vrp.D, 
                current_vertex,
                route.duration,
                free_vertices,
                [&vrp](Vertex u, Vertex v, double t) {
                    return vrp.TravelTime({u, v}, t);
                }
            );

            vector<Vertex> neighbors = V;
            // Remove all vertices that have INFTY makespan.
            neighbors.erase(remove_if(neighbors.begin(), neighbors.end(), 
                [&makespans_i_t](Vertex j) -> bool
                {
                    return makespans_i_t[j] == INFTY;
                }
            ), neighbors.end());
            // Remove the vertex itself
            neighbors.erase(remove(neighbors.begin(), neighbors.end(), current_vertex), neighbors.end());

            if (neighbors.empty()) // If no more vertices to visit, break.
            {
                clog << "*" << endl;
                break;
            }

            // Sort the vertices by makespan.
            sort(neighbors.begin(), neighbors.end(), 
                [&makespans_i_t](Vertex u, Vertex v) -> bool
                {
                    return makespans_i_t[u] < makespans_i_t[v];
                }
            );

            // Select closest (valid) vertex which is not the depot.
            Vertex next_vertex = vrp.d;
            for (Vertex j: neighbors)
            {
                if (j != vrp.d)
                {
                    next_vertex = j;
                    break;
                }
            }
            TimeUnit next_arrival_time = makespans_i_t[next_vertex];

            // Check if next vertex is the depot, close the route.
            if (next_vertex == vrp.d)
            {
                route.path.push_back(vrp.d);
                route.duration = next_arrival_time;
                break;
            }

            // Check if end depot is not reachable after the addition of the
            // next vertex, do not add it to the route, close the route.
            if (vrp.ArrivalTime({next_vertex, vrp.d}, next_arrival_time) == INFTY)
            {
                // Return to the depot and close this route.
                route.path.push_back(vrp.d);
                route.duration = vrp.ArrivalTime({current_vertex, vrp.d}, route.duration);
                break;
            }

            // Check capacity constraint.
            if (route_capacity + vrp.q[next_vertex] > vrp.Q)
            {
                // Return to the depot and close this route.
                route.path.push_back(vrp.d);
                route.duration = vrp.ArrivalTime({current_vertex, vrp.d}, route.duration);
                break;
            }
            route_capacity += vrp.q[next_vertex];

            clog << " -> " << next_vertex << " (arrival: "
                 << next_arrival_time
                 << ", route_cap: " << route_capacity << ")"; 
            route.path.push_back(next_vertex);
            route.duration = next_arrival_time;
            // Remove the vertex from the graph.
            visited_vertices.set(next_vertex);
        }

        // Store the route
        visited_vertices = unite(visited_vertices, route.path);
        // Reset depot visited vertices.
        visited_vertices.set(vrp.o, false);
        visited_vertices.set(vrp.d, false);

        routes.push_back(route);
        total_makespan += route.duration;
        clog << "GMH1: Route: " << route.path 
            << " -> Makespan: " << route.duration 
            << ", route capacity: " << route_capacity 
            << ", nb visited: " << route.path.size()
            << endl;
    }

    clog << "> Solution: " << routes.size() << " routes, Makespan: " << total_makespan << " - routes: " << routes << endl;
    return VRPSolution(total_makespan, routes);
}

/**
 * Converts a VRPSolution from Makespan to Duration.
 */
VRPSolution convert_makespan_solution_to_duration(
    const VRPSolution& makespan_solution,
    const VRPInstance& vrp
) {
    vector<Route> routes = vector<Route>(makespan_solution.routes.size());
    double total_duration = 0.0;
    for (size_t i = 0; i < makespan_solution.routes.size(); ++i)
    {
        routes[i] = vrp.BestDurationRoute(makespan_solution.routes[i].path);
        total_duration += routes[i].duration;
    }
    return VRPSolution(total_duration, routes);
}
 
/**
 * ### Computing the duration of routes provided by GMH1
 * 
 * All routes from GMH1 are valid, but all start at t=0.
 * Use the route paths to compute their corresponding optimal duration.
 */
VRPSolution ghm1_duration(
    const VRPInstance& vrp
) {
    const VRPSolution makespan_solution = greedy_makespan_heuristic_1(vrp);
    VRPSolution duration_solution = convert_makespan_solution_to_duration(makespan_solution, vrp);
    
    for (size_t i = 0; i < duration_solution.routes.size(); ++i)
    {
        const Route& route = duration_solution.routes[i];
        clog << "GMH1: Route: " << route.path 
            << " -> Duration: " << route.duration
            << ", nb visited: " << route.path.size()
            << endl;
    }
    clog << "> Solution: " << duration_solution.routes.size() << " routes, Duration: " << duration_solution.value << " - routes: " << duration_solution.routes << endl;
    return duration_solution;
}


} // namespace
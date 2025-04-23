#include "heuristics/aco.h"

#include <vector>

using namespace std;
using namespace goc;
using namespace nlohmann;

namespace solver
{

AntData::AntData():
    solution(VRPSolution(0.0, vector<Route>())),
    nb_visited_clients(0)
{}

/**
 * Initialize the candidates vector with all clients except the 
 * start depot and the preselected client.
 */
void AntData::init_candidates(Vertex preselected_client, int n)
{
    candidates = vector<Vertex>(n - 2); // exclude start depot and preselected client
    int index = 0;
    for (int i = 1; i < n; i++) { // skip start depot
        if (i == preselected_client) continue; // skip current
        candidates[index] = i;
        index++;
    }
}

/**
 * Fast remove a candidate vertex from the candidates vector.
 * Don't reallocate the vector, swap with the last element.
 */

double heuristic(
    const VRPInstance& vrp, 
    Vertex u, 
    Vertex v,
    double t
) {
    // Heuristic function: 1 / travel time
    double travel_time = vrp.TravelTime({u, v}, t);
    if (travel_time == INFTY) return 0.0;
    return 1.0 / travel_time;
}

/**
 * Calculate probabilities for each unvisited vertex
 * using the pheromone and heuristic information, and
 * select the next candidate vertex to visit.
 * 
 * Precondition: The last vertex in the currently 
 * built path is not the start depot.
 */
size_t select_next_candidate_index(
    Vertex current,
    double t,
    const vector<Vertex>& candidates,
    const vector<vector<double>>& pheromone,
    const VRPInstance& vrp,
    const AntColonyOptions& options
) {
    double sum = 0.0;
    size_t nb_candidates = candidates.size();
    double cumulative_numerator[nb_candidates];
    for (int i = 0; i < nb_candidates; i++) {
        Vertex candidate = candidates[i];
        sum = sum +
            (nyr::fast_pow(pheromone[current][candidate], options.alpha) * 
            nyr::fast_pow(heuristic(vrp, current, candidate, t), options.beta));
        cumulative_numerator[i] = sum;
    }

    // Randomly select the next vertex from candidates based on the probabilities
    double random = rand01();
    size_t selected_candidate_index = choose_candidate_index(
        cumulative_numerator, nb_candidates, r
    );
    return selected_candidate_index;
}

/**
 * ### Ant Colony Optimization (ACO)
 * 
 * Pure Makespan mode: each route starts at t=0.
 * Waiting is only useful to wait for TW ealiest arrivals
 * due to the FIFO property.
 */
void aco(
    nyr::TimedVrpSolution timed_solutions,
    const VRPInstance& vrp,
    const AntColonyOptions& options
) {
    const size_t n = vrp.D.NbVertices(); // \#{0, ..., n} = n = nb_clients + 2, depot is duplicated
    vector<vector<double>> pheromone(
        vrp.D.NbVertices(), 
        vector<double>(n, options.tau_0)
    );
    vector<AntData> ant_datas(
        options.nb_ants,
        AntData()
    );

    for(size_t iter = 0; iter < options.max_nb_iterations; ++iter)
    {
        for (size_t ant = 0; ant < options.nb_ants; ++ant)
        {
            AntData& data = ant_datas[ant];
            VRPSolution& sol = data.solution;
            
            // Reset the solution. But keep the allocated memory space used so far, to avoid reallocations.
            for (auto& route : sol.routes) {
                route.path.clear();
            }
            sol.routes.clear();
            sol.value = 0.0;

            // Randomly select a starting vertex after start depot
            // (random init arc from start depot)
            Vertex start_depot = vrp.o;
            Vertex current = static_cast<Vertex>(1 + rand() % (n - 1)); // exclude start and end depot
            sol.routes.push_back(
                Route(
                    {start_depot, current}, 
                    0.0, 
                    vrp.ArrivalTime({start_depot, current}, 0.0)
                )
            );
            data.nb_visited_clients = 1;
            CapacityUnit route_capacity = vrp.q[current];

            // While there are unvisited client vertices
            while (data.nb_visited_clients < n - 2)
            {
                // no self-loop possible since current  
                // is removed from candidates
                size_t next_candidate_index = select_next_candidate_index(
                    current,
                    sol.routes.back().duration, // ready time
                    data.candidates,
                    pheromone,
                    vrp,
                    options
                );
                Vertex next = candidates[next_candidate_index];

                if (next != vrp.d) {
                    // Check if end depot is not reachable after the addition of the
                    // next vertex, do not add it to the route, close the route instead.
                    if (vrp.ArrivalTime({next_vertex, vrp.d}, next_arrival_time) == INFTY)
                    {
                        next = vrp.d; // Return to the depot
                    }
                    // Check capacity constraint.
                    else if (route_capacity + vrp.q[next] > vrp.Q)
                    {
                        next = vrp.d;
                    }
                }

                // If next is end depot, close the current path
                if (next == vrp.d) {
                    // Close the route
                    sol.routes.back().path.push_back(vrp.d);
                    sol.routes.back().duration = vrp.ArrivalTime(
                        {current, vrp.d}, 
                        sol.routes.back().duration
                    );
                    sol.value += sol.routes.back().duration;

                    // Some candidate remains, start a new route
                    // Randomly select a starting vertex
                    do {
                        next =
                    route_capacity = 0.0;
                } else {
                    // Add the next vertex to the route
                    sol.routes.back().path.push_back(next);
                    sol.routes.back().duration = vrp.ArrivalTime(
                        {current, next}, 
                        sol.routes.back().duration
                    );
                    route_capacity += vrp.q[next];

                    // Remove selected candidate from the list
                    data.remove_visited_client(next_candidate_index);
                }
            }
        }
    }
    


}
}
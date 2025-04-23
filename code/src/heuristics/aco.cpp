#include "heuristics/aco.h"
#include "heuristics/greedy_makespan.h"

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
 * Binary search to find the index of the first element in p
 * that is greater than or equal to f.
 * 
 * For i in 0..nb_candidates-1], p[i] = sum_{j<i} tau1[j]^alpha
 * Returns k with probability (p[k]-p[k-1])/p[nb_candidates-1]
 * 
 * @param p: array of (cumulative) probabilities (not really probas, just sums)
 * @param nb_candidates: number of candidates
 * @param f: random number in [0,1]
 * @return: index of the selected candidate
 */
int choose_candidate_index(double* p, int nb_candidates, double f) {
    int left = 0;
    int right = nb_candidates - 1;
    int k;
    double total = p[nb_candidates - 1]; // sum of all probabilities in last element
    while (left<right){
        k = (left + right + 1) / 2; // round up 
        if (f < p[k-1] / total) right = k - 1;
        else if (f > p[k] / total) left = k + 1;
        else return k; 
    }
    return left;
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
    for (size_t i = 0; i < nb_candidates; i++) {
        Vertex candidate = candidates[i];
        sum = sum +
            (nyr::fast_pow(pheromone[current][candidate], options.alpha) * 
            nyr::fast_pow(heuristic(vrp, current, candidate, t), options.beta));
        cumulative_numerator[i] = sum;
    }

    // Randomly select the next vertex from candidates based on the probabilities
    double random = nyr::rand01();
    size_t selected_candidate_index = choose_candidate_index(
        cumulative_numerator, nb_candidates, random
    );
    return selected_candidate_index;
}

//TODO: complete the change
/**
 * ### Select the next valid candidate vertex
 * 
 * Precondition: current_vertex is set to not free.
 */
Vertex select_next_valid_candidate(
    const VRPInstance& vrp,
    Vertex current_vertex,
    double t,
    const VertexSet& free_vertices,
    const vector<vector<double>>& pheromone,
    const AntColonyOptions& options
) {
    // Compute EAT for each candidate vertex
    // NOTE: makespans_i_t has technically size n, all vertices
    // Unreached vertices have INFTY makespan.
    vector<double> makespans_i_t = compute_EAT_on_free_vertices(
        vrp.D, 
        current_vertex,
        t,
        free_vertices,
        [&vrp](Vertex u, Vertex v, double t) {
            return vrp.TravelTime({u, v}, t);
        }
    );

    // Remove all candidates that have INFTY makespan.
    vector<Vertex> neighbors = vrp.D.Vertices(); // copy of all vertices
    neighbors.erase(remove_if(neighbors.begin(), neighbors.end(), 
        [&makespans_i_t](Vertex v) -> bool
        {
            return makespans_i_t[v] == INFTY;
        }
    ), neighbors.end());

    // If no more candidates to visit, return depot
    if (neighbors.empty()) {
        return vrp.d;
    }

    // ACO selection
    double sum = 0.0;
    size_t nb_candidates = neighbors.size();
    double cumulative_numerator[nb_candidates];
    for (size_t i = 0; i < nb_candidates; i++) {
        Vertex candidate = neighbors[i];
        sum = sum +
            (nyr::fast_pow(pheromone[current_vertex][candidate], options.alpha) *
            nyr::fast_pow(1.0 / makespans_i_t[candidate], options.beta));
        cumulative_numerator[i] = sum;
    }

    // Randomly select the next vertex from candidates based on the probabilities
    double random = nyr::rand01();
    size_t selected_candidate_index = choose_candidate_index(
        cumulative_numerator, nb_candidates, random
    );
    Vertex next = neighbors[selected_candidate_index];
    return next;
}

Vertex select_start_vertex(
    vector<Vertex>& candidates,
    Vertex end_depot
) {
    // Randomly select a starting vertex that is not the end depot
    Vertex vertex;
    do {
        int random_candidate_index = nyr::rand_int(0, candidates.size() - 1);
        vertex = candidates[random_candidate_index];
    } while(vertex == end_depot); // ensure not depot
    return vertex;
}

/**
 * ### Ant Colony Optimization (ACO)
 * 
 * Pure Makespan mode: each route starts at t=0.
 * Waiting is only useful to wait for TW ealiest arrivals
 * due to the FIFO property.
 * 
 * WARNING: For now, once the routes are constructed in 
 * pure Makespan mode, the actual objective value is recomputed
 * to be Duration.
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
            data.init_candidates(current, n);
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

                #ifndef NDEBUG
                if (next_candidate_index >= data.candidates.size()) {
                    cerr << "next_candidate_index: " << next_candidate_index << endl;
                    cerr << "data.candidates.size(): " << data.candidates.size() << endl;
                    cerr << "data.candidates: " << data.candidates << endl;
                    cerr << "current: " << current << endl;
                    throw std::out_of_range("next_candidate_index out of range");
                }
                #endif

                Vertex next = data.candidates[next_candidate_index];

                if (next != vrp.d) {
                    // Check if end depot is not reachable after the addition of the
                    // next vertex, do not add it to the route, close the route instead.
                    TimeUnit next_arrival_time = vrp.ArrivalTime(
                        {current, next}, 
                        sol.routes.back().duration
                    );
                    //assert(next_arrival_time != INFTY && "next arrival time is INFTY");
                    #ifndef NDEBUG
                    if (next_arrival_time == INFTY) {
                        cerr << "next_arrival_time: " << next_arrival_time << endl;
                        cerr << "current: " << current << endl;
                        cerr << "next: " << next << endl;
                        throw std::runtime_error("next arrival time is INFTY");
                    }
                    #endif
                    
                    if (vrp.ArrivalTime({next, vrp.d}, next_arrival_time) == INFTY)
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
                // And start a new route
                if (next == vrp.d) {
                    data.close_path(vrp, current);

                    // Some candidate remains, start a new route
                    // Randomly select an (unvisited) starting vertex after start depot
                    next = select_start_vertex(data.candidates, vrp.d);
                    sol.routes.push_back(
                        Route(
                            {start_depot},
                            0.0,
                            0.0
                        )
                    );
                    route_capacity = 0.0;
                }
                // Add the next (non-depot) vertex to the route
                sol.routes.back().path.push_back(next);
                sol.routes.back().duration = vrp.ArrivalTime(
                    {current, next}, 
                    sol.routes.back().duration
                );
                route_capacity += vrp.q[next];

                // Remove selected candidate from the list
                data.remove_visited_client(next_candidate_index);
                current = next;
            }
            // Close the last route
            data.close_path(vrp, current);

            // TODO: To improve
            // Convert from Makespan to Duration
            data.solution = convert_makespan_solution_to_duration(
                data.solution, 
                vrp
            );
        }

        // Evaporate pheromones
        for (size_t i = 0; i < pheromone.size(); ++i)
        {
            for (size_t j = 0; j < pheromone.size(); ++j)
            {
                double new_val = pheromone[i][j] * (1.0 - options.rho);
                pheromone[i][j] = bound_pheromone_val(
                    new_val, 
                    options.tau_min, 
                    options.tau_max
                );
            }
        }

        // Deposit pheromone based on solution quality
        // Also find if a new best solution was found
        size_t best_ant = 0;
        bool found_new_best = false;
        double best_value = timed_solutions.last_solution_value();
        nyr::Durex time_to_best;
        for (size_t ant = 0; ant < options.nb_ants; ++ant)
        {
            AntData& data = ant_datas[ant];
            VRPSolution& sol = data.solution;
            double delta_tau = 1.0 / sol.value; // inverse solution quality
            for (auto& route : sol.routes)
            {
                for (size_t i = 0; i < route.path.size() - 1; ++i)
                {
                    #ifndef NDEBUG
                    if (route.path[i] == route.path[i + 1]) {
                        cerr << "Self-loop detected in route: " << route.path << endl;
                        throw std::runtime_error("Self-loop detected in route");
                    }
                    if (route.path[i] >= pheromone.size() || route.path[i + 1] >= pheromone.size()) {
                        cerr << "Invalid vertex index in route: " << route.path << endl;
                        throw std::out_of_range("Invalid vertex index in route");
                    }
                    #endif

                    Vertex u = route.path[i];
                    Vertex v = route.path[i + 1];
                    double new_val = pheromone[u][v] + delta_tau;
                    pheromone[u][v] = bound_pheromone_val(
                        new_val, 
                        options.tau_min, 
                        options.tau_max
                    );
                }
            }

            // Check if this ant has the best solution so far
            if (sol.value < best_value)
            {
                best_value = sol.value;
                best_ant = ant;
                found_new_best = true;
                time_to_best = timed_solutions.pclock.elapsed();
            }
        }

        // If a new best solution was found, add it to the timed solutions
        if (found_new_best)
        {
            auto& best_solution = ant_datas[best_ant].solution;
            timed_solutions.add(time_to_best, best_solution);
        }
    }
}
}
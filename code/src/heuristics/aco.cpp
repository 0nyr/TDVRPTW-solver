#include "heuristics/aco.h"
#include "heuristics/greedy_makespan.h"
#include "nyr/solutions/conversions.h"

#include <vector>

using namespace std;
using namespace goc;
using namespace nyr;
using namespace nlohmann;

//#define PRINT_ACO

namespace solver
{

AntCandidates::AntCandidates():
    solution(VRPSolutionMakespan(
        0.0, 
        vector<RouteMakespan>()
    )),
    nb_visited_clients(0)
{}

/**
 * ### Remove a candidate client from the list of candidates
 * 
 */
inline void AntCandidates::remove_visited_client(goc::Vertex removed_candidate)
{
    // Find the index of the removed candidate
    auto it = std::find(candidates.begin(), candidates.end(), removed_candidate);
    if (it != candidates.end()) {
        remove_candidate(
            std::distance(candidates.begin(), it),
            removed_candidate
        );
    } else {
        std::cerr << "Error: Removed candidate not found in candidates." <<  std::endl;
        std::cerr << "Removed candidate: " << removed_candidate <<  std::endl;
        std::cerr << "Candidates: " << candidates <<  std::endl;
        std::cerr << "Free vertices: " << free_vertices <<  std::endl;
        throw std::runtime_error("Removed candidate not found in candidates");
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

vector<double> compute_AT_on_free_vertices(
    const VRPInstance& vrp,
    Vertex current_vertex,
    double t,
    const VertexSet& free_vertices
) {
    // Compute EAT for each candidate vertex
    // NOTE: arrival_time_i_t has technically size n, all vertices
    // Unreached vertices have INFTY makespan.
    vector<double> arrival_time_i_t = vector<double>(vrp.D.NbVertices(), INFTY);
    for (Vertex v : vrp.D.Vertices()) {
        if (contains(free_vertices, v)) {
            // Compute the arrival time for each free vertex
            arrival_time_i_t[v] = vrp.ArrivalTime({current_vertex, v}, t);
        }
    }
    return arrival_time_i_t;
}

double heuristic_wrapper(
    double min_makespan,
    double value
) {
    if (value <= 0.0 || value >= INFTY)
        return 0.0;
    if (min_makespan <= 0.0 || min_makespan >= INFTY)
        return 1.0 / value;
    return (min_makespan / value);
}

/**
 * ### Select the next valid candidate vertex
 * 
 * Precondition: current_vertex is set to not free.
 */
Vertex select_next_valid_candidate_from_EAT(
    const VRPInstance& vrp,
    const vector<Vertex>& candidates,
    const Vertex current_vertex,
    const CapacityUnit route_capacity,
    const double t,
    const VertexSet& free_vertices,
    const vector<vector<double>>& pheromone,
    const AntColonyOptions& options
) {
    // TODO: Make an alternative version using only arrival times
    // TODO: Make a more optimized version returning an array of EAT only for the free vertices.
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
    vector<double> arrival_time_i_t = compute_AT_on_free_vertices(
        vrp,
        current_vertex,
        t,
        free_vertices
    );

    // Remove all candidates that have INFTY AT.
    vector<Vertex> neighbors = vrp.D.Vertices(); // copy of all vertices
    neighbors.erase(remove_if(neighbors.begin(), neighbors.end(), 
        [&arrival_time_i_t](Vertex v) -> bool
        {
            return arrival_time_i_t[v] == INFTY;
        }
    ), neighbors.end());

    // If no more candidates to visit, return end depot
    if (neighbors.empty()) {
        return vrp.d;
    }

    // ACO: Creating proba distributions based on pheromone and heuristic
    double min_makespan = *std::min_element(makespans_i_t.begin(), makespans_i_t.end());
    double sum = 0.0;
    size_t nb_candidates = neighbors.size();
    double cumulative_numerator[nb_candidates];
    for (size_t i = 0; i < nb_candidates; i++) {
        Vertex candidate = neighbors[i];
        sum = sum +
            (nyr::fast_pow(pheromone[current_vertex][candidate], options.alpha) *
            nyr::fast_pow(heuristic_wrapper(min_makespan, makespans_i_t[candidate]), options.beta));
        cumulative_numerator[i] = sum;
    }

    #ifdef PRINT_ACO_DETAILED
    // print probas
    vector<double> cands = vector<double>(neighbors.size());
    vector<double> probs = vector<double>(neighbors.size());
    double preced = 0.0;
    for (size_t i = 0; i < nb_candidates; i++) {
        cands[i] = neighbors[i];
        probs[i] = (cumulative_numerator[i] - preced) / cumulative_numerator[nb_candidates - 1];
        preced = cumulative_numerator[i];
    }

    print_padded_vectors(
        std::clog, 
        cands, 
        probs
    );
    #endif

    // ACO: Randomly select the next vertex from candidates based on the probabilities
    double random = nyr::rand01();
    size_t selected_candidate_index = choose_candidate_index(
        cumulative_numerator, nb_candidates, random
    );
    Vertex next = neighbors[selected_candidate_index];

    // Post selection checks to ensure validity
    if (next != vrp.d) {
        // Check if end depot is not reachable after the addition of the
        // next vertex, do not add it to the route, close the route instead.
        // TODO: variant: check if it is better to come back to the depot than next, use EAT for that.
        TimeUnit next_arrival_time = arrival_time_i_t[next];
        assert(next_arrival_time != INFTY && "next arrival time is INFTY");
        #ifndef NDEBUG
        TimeUnit next_arrival_time2 = vrp.ArrivalTime(
            {current_vertex, next}, 
            t
        );
        if (next_arrival_time != next_arrival_time2) {
            std::cerr << "Error: Arrival time mismatch." << std::endl;
            std::cerr << "Current vertex: " << current_vertex << std::endl;
            std::cerr << "Next vertex: " << next << std::endl;
            std::cerr << "Arrival time: " << next_arrival_time << std::endl;
            std::cerr << "Expected arrival time: " << next_arrival_time2 << std::endl;
            throw std::runtime_error("Arrival time mismatch");
        }
        #endif
        
        if (vrp.ArrivalTime({next, vrp.d}, next_arrival_time) == INFTY)
        {
            return vrp.d; // Return to the depot
        }
        // Check capacity constraint.
        else if (route_capacity + vrp.q[next] > vrp.Q)
        {
            return vrp.d;
        }
    }

    return next;
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
ACOStatus aco(
    nyr::AbstractSolutionRecord& solution_record, 
    const VRPInstance& vrp,
    const AntColonyOptions& options
) {
    const size_t n = vrp.D.NbVertices(); // \#{0, ..., n} = n = nb_clients + 2, depot is duplicated
    vector<vector<double>> pheromone(
        vrp.D.NbVertices(), 
        vector<double>(n, options.tau_0)
    );
    vector<AntCandidates> ant_datas(
        options.nb_ants,
        AntCandidates()
    );
    
    
        
    double sum_pheromones_last_iter = 0.0; // Sum to compute the the variation of pheromones since last iteration
    size_t no_improvement_iter = 0;

    for(size_t iter = 0; iter < options.max_nb_iterations; ++iter)
    {
        for (size_t ant = 0; ant < options.nb_ants; ++ant)
        {
            AntCandidates& data = ant_datas[ant];
            VRPSolution& sol = data.solution;
            
            // Reset the solution. But keep the allocated memory space used so far, to avoid reallocations.
            for (auto& route : sol.routes) {
                route.path.clear();
            }
            sol.routes.clear();
            sol.value = 0.0;

            // Initialization
            data.init_candidates(vrp);
            Vertex current = vrp.d; // Needed to start a first route
            CapacityUnit route_capacity = 0.0;
            double t = 0.0;

            // While there are unvisited client vertices
            while (data.nb_visited_clients < n - 2)
            {
                // Check if a new route is needed
                if (current == vrp.d) {
                    // Start a new route
                    data.open_path(vrp);
                    current = vrp.o;
                    route_capacity = 0.0;
                    t = 0.0;
                }

                // no self-loop possible since current removed from candidates
                Vertex next = select_next_valid_candidate_from_EAT(
                    vrp,
                    data.candidates,
                    current,
                    route_capacity,
                    t,
                    data.free_vertices,
                    pheromone,
                    options
                );
                #ifdef PRINT_ACO_DETAILED
                // print selected candidate
                std::clog << current << " -> " << next << " - remaining candidates: " 
                    << data.free_vertices.count() 
                    << endl;
                #endif

                // If next is end depot, close the current path
                if (next == vrp.d) {
                    data.close_path(vrp, current);
                } else {
                    assert(next != current && "Self-loop detected");
                    assert(current == sol.routes.back().path.back() 
                        && "Current vertex should be the last vertex in the route");
                    // Add the next (non-depot) vertex to the route
                    TimeUnit next_arrival_time = vrp.ArrivalTime(
                        {current, next}, 
                        t
                    );
                    #ifndef NDEBUG
                    if (next_arrival_time == INFTY) {
                        std::cerr << "Error: Arrival time is INFTY." << std::endl;
                        std::cerr << "Current vertex: " << current << std::endl;
                        std::cerr << "Next vertex: " << next << std::endl;
                        std::cerr << "All routes: " << sol.routes << std::endl;
                        std::cerr << "t: " << t << std::endl;
                        throw std::runtime_error("Arrival time is INFTY");
                    }
                    #endif
                    sol.routes.back().path.push_back(next);
                    sol.routes.back().duration = next_arrival_time;
                    t = next_arrival_time;
                    route_capacity += vrp.q[next];
                    try {
                        data.remove_visited_client(
                            next
                        );
                    } catch (const std::runtime_error& e) {
                        std::cerr << "Error: " << e.what() << std::endl;
                        std::cerr << "Current vertex: " << current << std::endl;
                        std::cerr << "Next vertex: " << next << std::endl;
                        std::cerr << "All routes: " << sol.routes << std::endl;
                        throw;
                    }
                    
                }

                current = next;
            }
            // Close the last route
            data.close_path(vrp, current);

            // Convert from Makespan to Duration
            data.solution = convert_makespan_solution_to_duration(
                data.solution, 
                vrp
            );

            // Print current solution
            #ifdef PRINT_ACO
            clog << "Ant " << ant << ": " 
                << "Solution: " << sol.routes.size() 
                << " routes, Value: " << sol.value 
                << " - routes: " << sol.routes 
                << endl;
            #endif
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
        double best_value = solution_record.last_solution_value();
        nyr::Durex time_to_best;
        for (size_t ant = 0; ant < options.nb_ants; ++ant)
        {
            AntCandidates& data = ant_datas[ant];
            VRPSolution& sol = data.solution;
            double delta_tau = 1.0 / sol.value; // inverse solution quality
            for (auto& route : sol.routes)
            {
                for (size_t i = 0; i < route.path.size() - 1; ++i)
                {
                    #ifndef NDEBUG
                    if (route.path[i] == route.path[i + 1]) {
                        cerr << "Self-loop detected in route: " << route.path << endl;
                        cerr << "complete solution: " << sol << endl;
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
                time_to_best = solution_record.pclock.elapsed();
            }
        }
        // If a new best solution was found, add it to the timed solutions
        if (found_new_best)
        {
            auto& best_solution = ant_datas[best_ant].solution;
            solution_record.add(time_to_best, best_solution, "ACO");
        }

        // Compute pheromone delta since last iteration
        double current_pheromone_sum = 0.0;
        for (size_t i = 0; i < pheromone.size(); ++i)
        {
            for (size_t j = 0; j < pheromone.size(); ++j)
            {
                current_pheromone_sum += pheromone[i][j];
            }
        }
        double delta_pheromone = abs(current_pheromone_sum - sum_pheromones_last_iter);
        #ifdef PRINT_ACO
        clog << "Delta pheromone: " << delta_pheromone << endl;
        #endif
        sum_pheromones_last_iter = current_pheromone_sum;

        if (delta_pheromone < options.delta_pheromone_threshold)
        {
            // Start incrementing no-improvement iterations
            no_improvement_iter++;
        }
        if (no_improvement_iter >= options.max_no_improvement)
        {
            // Stop the algorithm if no improvement for too long
            clog << "No improvement for " << no_improvement_iter << " iterations, stopping ACO." << endl;
            return ACOStatus::NoImprovement;
        }

        // Stop the algorithm if time limit is reached
        if (solution_record.pclock.elapsed() >= options.gparams.time_limit)
        {
            clog << "Time limit reached, stopping ACO." << endl;
            return ACOStatus::TimeLimitReached;
        }
    }

    return ACOStatus::Finished;
}
}
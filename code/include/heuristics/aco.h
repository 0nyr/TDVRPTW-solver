#pragma once

#include <cstdint>
#include <nyr/nyr.h>

#include <goc/goc.h>
#include "nyr/vrp/instance.h"

namespace solver
{
// All the status of the ACO algorithm.
enum class ACOStatus
{
    Finished, // The algorithm has finished.
    Converged, // The algorithm has converged.
    NoImprovement, // The algorithm has stopped due to no improvement.
    TimeLimitReached // The algorithm has stopped due to global time limit.
};

class AntCandidates
{
public:
    uint32_t nb_visited_clients; // number of visited clients
    std::vector<goc::Vertex> candidates; // candidate vertices to visit
    nyr::VertexSet free_vertices; // free vertices to visit

    AntCandidates();

    /**
     * ### Remove candidate client vertex
     */
    inline void remove_candidate(
        size_t candidate_index,
        goc::Vertex removed_candidate
    ) {
        // Swap the removed candidate with the last candidate
        assert(candidate_index < candidates.size());
        assert(candidates[candidate_index] == removed_candidate);
        candidates[candidate_index] = candidates.back();
        candidates.pop_back(); // remove last element (start depot)
        nb_visited_clients++;
        free_vertices.set(removed_candidate, false); // mark the removed candidate as visited
    }

    /**
     * ### Initialize the ant data
     */
    inline void init_candidates(const nyr::VRPInstance& vrp, bool remove_end_depot = true)
    {
        candidates = vrp.D.Vertices(); // copy all vertices
        free_vertices = nyr::VertexSet().set(); // start with all vertices as free
        
        assert(candidates[0] == vrp.o);
        assert(candidates.back() == vrp.d);

        // Remove the end depot from the candidates
        if (remove_end_depot) {
            candidates.pop_back(); 
            free_vertices.set(vrp.d, false);
        }

        // remove-swap the start depot
        candidates[0] = candidates.back();
        candidates.pop_back(); // remove last element (start depot)
        free_vertices.set(vrp.o, false); // start depot is not free
    
        nb_visited_clients = 0; // no clients visited yet
    }
    
    void remove_visited_client(goc::Vertex removed_candidate);
};

inline double bound_pheromone_val(
    double new_val,
    double tau_min,
    double tau_max
) {
    if (new_val < tau_min)
        return tau_min;
    else if (new_val > tau_max)
        return tau_max;
    else
        return new_val;
}

nyr::VRPSolutionMakespan build_ant_solution(
    const nyr::VRPInstance& vrp,
    const std::vector<std::vector<double>>& pheromones,
    const nyr::AntColonyParams& options
);

void evaporate_pheromones(
    std::vector<std::vector<double>>& pheromones,
    const nyr::AntColonyParams& options
);

/**
 * ### Ant Colony Optimization (ACO)
 * 
 * Heuristic solution construction: each route starts at t=0.
 * Waiting is only useful to wait for TW ealiest arrivals
 * due to the FIFO property.
 * 
 * Solution conversion happens if needed.
 */
template<typename Solution>
ACOStatus aco(
    nyr::SolutionRecord<Solution>& solution_record, 
    const nyr::VRPInstance& vrp,
    const nyr::AntColonyParams& options
) {
    const size_t n = vrp.D.NbVertices(); // \#{0, ..., n} = n = nb_clients + 2, depot is duplicated
    std::vector<std::vector<double>> pheromones(
        n, 
        std::vector<double>(n, options.tau_0)
    );
    std::vector<Solution> solutions(
        options.nb_ants
    );
    double sum_pheromones_last_iter = 0.0; // Sum to compute the the variation of pheromones since last iteration
    size_t no_improvement_iter = 0;

    for(size_t iter = 0; iter < options.max_nb_iterations; ++iter)
    {
        for (size_t ant = 0; ant < options.nb_ants; ++ant)
        {
            const nyr::VRPSolutionMakespan sol = build_ant_solution(vrp, pheromones, options);
            solutions[ant] = nyr::auto_convert_makespan_solution<Solution>(sol, vrp);
        }

        evaporate_pheromones(pheromones, options);

        // Deposit pheromones based on solution quality
        // Also find if a new best solution was found
        size_t best_ant = 0;
        bool found_new_best = false;
        double best_value = solution_record.last_solution_value();
        nyr::Durex time_to_best;
        for (size_t ant = 0; ant < options.nb_ants; ++ant)
        {
            Solution& sol = solutions[ant];
            double delta_tau = 1.0 / sol.value; // inverse solution quality
            for (auto& route : sol.routes)
            {
                for (size_t i = 0; i < route.path.size() - 1; ++i)
                {
                    #ifndef NDEBUG
                    if (route.path[i] == route.path[i + 1]) {
                        std::cerr << "Self-loop detected in route: " << route << std::endl;
                        std::cerr << "complete solution: " << sol << std::endl;
                        throw std::runtime_error("Self-loop detected in route");
                    }
                    if (route.path[i] >= pheromones.size() || route.path[i + 1] >= pheromones.size()) {
                        std::cerr << "Invalid vertex index in route: " << route << std::endl;
                        throw std::out_of_range("Invalid vertex index in route");
                    }
                    #endif

                    goc::Vertex u = route.path[i];
                    goc::Vertex v = route.path[i + 1];
                    double new_val = pheromones[u][v] + delta_tau;
                    pheromones[u][v] = bound_pheromone_val(
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
            auto& best_solution = solutions[best_ant];
            solution_record.add(time_to_best, best_solution, "ACO");
        }

        // Compute pheromones delta since last iteration
        double current_pheromone_sum = 0.0;
        for (size_t i = 0; i < pheromones.size(); ++i)
        {
            for (size_t j = 0; j < pheromones.size(); ++j)
            {
                current_pheromone_sum += pheromones[i][j];
            }
        }
        double delta_pheromone = abs(current_pheromone_sum - sum_pheromones_last_iter);
        #ifdef PRINT_ACO
        clog << "Delta pheromones: " << delta_pheromone << endl;
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
            std::clog << "No improvement for " << no_improvement_iter << " iterations, stopping ACO." << std::endl;
            return ACOStatus::NoImprovement;
        }

        // Stop the algorithm if time limit is reached
        if (solution_record.pclock.elapsed() >= options.gparams.time_limit)
        {
            std::clog << "Time limit reached, stopping ACO." << std::endl;
            return ACOStatus::TimeLimitReached;
        }
    } // end of iteration loop

    return ACOStatus::Finished;
}

} // namespace solver
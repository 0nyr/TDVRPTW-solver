#pragma once

#include <cstdint>
#include <nyr/nyr.h>

#include <goc/goc.h>
#include "instance/vrp_instance.h"

namespace solver
{

class AntColonyOptions: public goc::Printable
{
public:
    const uint64_t max_nb_iterations; // maximum number of iterations
    const uint64_t max_no_improvement; // maximum number of iterations without improvement
    const uint32_t nb_ants; // number of ants
    const uint32_t alpha; // pheromone importance
    const uint32_t beta; // heuristic importance
    const double rho; // pheromone evaporation rate
    const double tau_min; // minimum pheromone level
    const double tau_0; // initial pheromone level
    const double tau_max; // maximum pheromone level
    const double delta_pheromone_threshold; // pheromone threshold for determining convergence

    // Constructor to initialize all const members
    AntColonyOptions(
        uint64_t max_nb_iterations,
        uint64_t max_no_improvement,
        uint32_t nb_ants,
        uint32_t alpha,
        uint32_t beta,
        double rho,
        double tau_min,
        double tau_0,
        double tau_max,
        double delta_pheromone_threshold
    ):
        max_nb_iterations(max_nb_iterations),
        max_no_improvement(max_no_improvement),
        nb_ants(nb_ants),
        alpha(alpha),
        beta(beta),
        rho(rho),
        tau_min(tau_min),
        tau_0(tau_0),
        tau_max(tau_max),
        delta_pheromone_threshold(delta_pheromone_threshold)
    {}

    // Print ACO options
    void Print(std::ostream& os) const override
    {
        os << "Ant Colony Options:" << std::endl;
        os << "  max_nb_iterations: " << max_nb_iterations << std::endl;
        os << "  max_no_improvement: " << max_no_improvement << std::endl;
        os << "  nb_ants: " << nb_ants << std::endl;
        os << "  alpha: " << alpha << std::endl;
        os << "  beta: " << beta << std::endl;
        os << "  rho: " << rho << std::endl;
        os << "  tau_min: " << tau_min << std::endl;
        os << "  tau_0: " << tau_0 << std::endl;
        os << "  tau_max: " << tau_max << std::endl;
        os << "  delta_pheromone_threshold: " << delta_pheromone_threshold << std::endl;
    }
};

class AntData
{
public:
    goc::VRPSolution solution; // solution built by the ant
    uint32_t nb_visited_clients; // number of visited clients
    std::vector<goc::Vertex> candidates; // candidate vertices to visit
    VertexSet free_vertices; // free vertices to visit

    AntData();

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
     * 
     * Ramdomly select a starting vertex from the candidates.
     * Initialize the candidates vector with all clients except the 
     * start depot and the preselected client.
     */
    inline void init_candidates(const VRPInstance& vrp, bool remove_end_depot = true)
    {
        candidates = vrp.D.Vertices(); // copy all vertices

        assert(candidates[0] == vrp.o);
        assert(candidates.back() == vrp.d);

        // Remove the end depot from the candidates
        if (remove_end_depot) {
            candidates.pop_back(); // remove end depot
        }

        // remove-swap the start depot
        candidates[0] = candidates.back();
        candidates.pop_back(); // remove last element (start depot)

        free_vertices = VertexSet().set(); // start with all vertices as free
        free_vertices.set(vrp.o, false); // start depot is not free
    
        nb_visited_clients = 0; // no clients visited yet
    }

    goc::Vertex open_path(const VRPInstance& vrp);
    
    void remove_visited_client(goc::Vertex removed_candidate);

    /**
     * ### Close last path to make it a route
     */
    inline void close_path(const VRPInstance& vrp, goc::Vertex current)
    {
        solution.routes.back().path.push_back(vrp.d);
        solution.routes.back().duration = vrp.ArrivalTime(
            {current, vrp.d}, 
            solution.routes.back().duration
        );
        solution.value += solution.routes.back().duration;
    }
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

void aco(
    nyr::TimedVrpSolution timed_solutions,
    const VRPInstance& vrp,
    const AntColonyOptions& options
);

} // namespace solver
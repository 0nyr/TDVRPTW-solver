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

    // Constructor to initialize all const members
    AntColonyOptions(
        uint64_t max_nb_iterations,
        uint64_t max_no_improvement,
        uint32_t nb_ants,
        uint32_t alpha = 1,
        uint32_t beta = 2,
        double rho = 0.05,
        double tau_min = 0.000001,
        double tau_0 = 1.0,
        double tau_max = 10.0
    ): 
        max_nb_iterations(max_nb_iterations),
        max_no_improvement(max_no_improvement),
        nb_ants(nb_ants),
        alpha(alpha),
        beta(beta),
        rho(rho),
        tau_min(tau_min),
        tau_0(tau_0),
        tau_max(tau_max)
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
    }
};

class AntData
{
public:
    goc::VRPSolution solution; // solution built by the ant
    uint32_t nb_visited_clients; // number of visited clients
    std::vector<goc::Vertex> candidates; // candidate vertices to visit
    //VertexSet free_vertices; // free vertices to visit

    AntData();
    void init_candidates(goc::Vertex preselected_client, int n);
    
    /**
     * ### Remove a candidate from the list of candidates
     * 
     * Efficiently removes a candidate from the list of candidates.
     * The last candidate and the removed candidate are swapped.
     * This is done to avoid shifting all elements in the vector.
     */
    inline void remove_visited_client(size_t candidate_index)
    {
        assert(candidate_index < candidates.size() && "Candidate index out of range");
        //goc::Vertex removed_candidate = candidates[candidate_index];
        candidates[candidate_index] = candidates.back(); // move last element into the removed slot
        candidates.pop_back(); // logically shrink vector

        nb_visited_clients++;
        //free_vertices.set(removed_candidate, false); // mark the removed candidate as visited
    }

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
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
    const double alpha; // pheromone importance
    const double beta; // heuristic importance
    const double rho; // pheromone evaporation rate
    const double tau_min; // minimum pheromone level
    const double tau_0; // initial pheromone level
    const double tau_max; // maximum pheromone level

    // Constructor to initialize all const members
    AntColonyOptions(
        uint64_t max_nb_iterations,
        uint64_t max_no_improvement,
        uint32_t nb_ants,
        double alpha = 1.0,
        double beta = 2.0,
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
    VRPSolution solution; // solution built by the ant
    uint32_t nb_visited_clients; // number of visited clients
    vector<Vertex> candidates; // candidate vertices to visit

    AntData();
    void init_candidates(Vertex preselected_client, int n);
};

} // namespace solver
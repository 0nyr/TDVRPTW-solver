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

            // Randomly select a starting vertex 
            // (random init arc from start depot)
            Vertex start_depot = vrp.o;
            Vertex current = static_cast<Vertex>(1 + rand() % (n - 1)); // exclude start and end depot
            sol.routes.push_back(Route({start_depot, current}, 0.0, vrp.ArrivalTime({start_depot, current}, 0.0)));
        }
    }
        


}
}
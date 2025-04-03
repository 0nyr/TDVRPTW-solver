#include "labeling/ng_neighborhoods.h"
#include "goc/math/math_utils.h"
#include "goc/collection/bitset_utils.h"

using namespace std;
using namespace goc;

namespace solver
{
namespace
{
// Solve a one-to-all makespan minimization time dependent 
// shortest path for a given vertex (source), at a given departure time.
// Returns: the optimal makespan of every other vertex from the source.
// NOTE: Due to FIFO property, waiting is not allowed.
// WARNING: Different from compute_EAT_from_departure_time
// because this function considers the time windows.
vector<double> compute_one_to_all_earliest_arrival_time(
    const VRPInstance& vrp,
    Vertex source,
    TimeUnit departure_time
) {
    int n = vrp.D.NbVertices();

    if (vrp.tw[source].right < departure_time)
        // The source is not reachable at the given departure time
        return {};

    vector<double> arrival_times(n, INFTY); // all arrival times are in [0, INFTY).
    arrival_times[source] = max(departure_time, vrp.tw[source].left); // arrival time, considering no waiting time and start at departure_time.

    // Priority queue to select the vertex with the smallest arrival time.
    priority_queue<pair<double, Vertex>, vector<pair<double, Vertex>>, greater<pair<double, Vertex>>> q;
    q.push({arrival_times[source], source});

    // TD Dijkstra's algorithm, no "visited" vector needed.
    while (!q.empty())
    {
        auto [arrival_time_at_i, i] = q.top();
        q.pop();

        // Update the makespan of the successors.
        for (Vertex j: vrp.D.Successors(i))
        {
            // Note: TW and service times are already considered in the function.
            double arrival_time_at_j = vrp.ArrivalTime({i, j}, arrival_time_at_i);
            // Check for improvement, and TW feasibility.
            if (arrival_time_at_j < arrival_times[j])
            {
                arrival_times[j] = arrival_time_at_j;
                q.push({arrival_time_at_j, j});
            }
        }
    }

    return arrival_times;
}
} // anonymous namespace

PartitionedInterval partition_time_horizon(
    const Interval& horizon,
    const vector<Interval>& time_steps,
    NHPS horizon_partitioning_strategy
) {
    // Precondition: If there is only one time step, then the horizon is the only interval.
    if (time_steps.size() <= 1)
    {
        return PartitionedInterval(horizon);
    }

    if (horizon_partitioning_strategy == NHPS::TimeStepSpecific)
    {
        // Reuse the time steps directly.
        return PartitionedInterval(time_steps);
    }
    else if (horizon_partitioning_strategy == NHPS::PartitionedHorizon)
    {
        // Aggregate the time steps.
        size_t nb_partitions_of_horizon = fast_log2(time_steps.size()) + 1; // better than just dividing by some value.
        
        vector<double> breakpoints;
        // Add first breakpoint.
        breakpoints.push_back(horizon.left);
        
        // Add the rest of the breakpoints, except the last one.
        double period_duration = (horizon.right - horizon.left) / nb_partitions_of_horizon;
        for (size_t i = 1; i < nb_partitions_of_horizon - 1; ++i)
        {
            breakpoints.push_back(i*period_duration + horizon.left);
        }
        // Add last breakpoint.
        breakpoints.push_back(horizon.right);

        assert (breakpoints.size() == nb_partitions_of_horizon && "Number of breakpoints must be equal to the number of partitions of the horizon.");
        
        return PartitionedInterval(breakpoints);
    }
    else if (horizon_partitioning_strategy == NHPS::Static)
    {
        // Only one interval: the horizon.
        return PartitionedInterval(horizon);
    }
    else
    {
        throw runtime_error("Unknown time strategy.");
    }
}

TDNGNeighborhoods::TDNGNeighborhoods(
    const VRPInstance& vrp, 
    const goc::PartitionedInterval& partitioned_horizon,
    uint32_t nb_neighbors_to_keep
):
    partitioned_horizon_(partitioned_horizon)
{
    const int n = vrp.D.NbVertices();
	const auto& V = vrp.D.Vertices();
    const int nb_horizon_partitions = partitioned_horizon.nb_intervals();
    ng_td_neighborhoods_ = vector<TDNeighborhoods>(n, // for each vertex
        vector<VertexSet>(nb_horizon_partitions, // for each time period
            VertexSet() // neighbors, empty (no neighbors) by default
        )
    );
    clog << "TD NG Neighbors processing..." << endl;
    for (Vertex i: V)
    {
        for (int t = 0; t < nb_horizon_partitions; ++t)
        {   
            // Step 2: Solve a one-to-all makespan minimization 
            // time dependent shortest path for each vertex, 
            // for each time period.
            vector<double> makespans_i_t = compute_one_to_all_earliest_arrival_time(
                vrp, 
                i, 
                partitioned_horizon.get_interval(t).left
            );
            if (makespans_i_t.empty()) continue; // The vertex is not reachable at the given departure time for the given time period.

            // Step 3: Determine the neighborhoods, i.e., the closest 
            // neighbors for each vertex, for each time period.
            vector<Vertex> neighbors = V;
            // Remove all vertices that have INFTY makespan.
            neighbors.erase(remove_if(neighbors.begin(), neighbors.end(), 
                [&makespans_i_t](Vertex j) -> bool
                {
                    return makespans_i_t[j] == INFTY;
                }
            ), neighbors.end());
            if (neighbors.empty()) continue; // No neighbors for the vertex.
            
            // Sort the vertices by makespan.
            sort(neighbors.begin(), neighbors.end(), 
                [&makespans_i_t](Vertex u, Vertex v) -> bool
                {
                    return makespans_i_t[u] < makespans_i_t[v];
                }
            );

            // print each vertex and its makespan
            clog << " - Vertex " << i << " in period " << t << " at time " << partitioned_horizon.get_interval(t).left << " makespans: ";
            for (Vertex j: neighbors)
            {
                clog << j;
                if (j == i) clog << " (self)";
                clog << " -> " << makespans_i_t[j];
                if (j != neighbors.back()) clog << ", ";
            }
            clog << endl;
            // Remove the vertex itself
            neighbors.erase(remove(neighbors.begin(), neighbors.end(), i), neighbors.end());

            // Keep only the closest neighbors.
            if (neighbors.size() > nb_neighbors_to_keep)
                neighbors.resize(nb_neighbors_to_keep);
            
            // Store the neighbors.
            ng_td_neighborhoods_[i][t] = create_bitset<MAX_N>(neighbors);
        }
    }

    // For comparison: compare with using compute_earliest_arrival_time
    // for each vertex, for each time period.
    vector<vector<VertexSet>> EATs_neighborhoods(n, vector<VertexSet>(nb_horizon_partitions, VertexSet()));
    clog << "EATs NG Neighbors processing..." << endl;
    for (Vertex i: V)
    {
        for (int t = 0; t < nb_horizon_partitions; ++t)
        {
            vector<double> makespans_i_t = compute_earliest_arrival_time(
                vrp.D, 
                i, 
                partitioned_horizon.get_interval(t).left, 
                [&] (Vertex u, Vertex v, double t0) {
                    return vrp.TravelTime({u, v}, t0);
                }
            );

            // do same as before
            if (makespans_i_t.empty()) continue; // The vertex is not reachable at the given departure time for the given time period.

            // Step 3: Determine the neighborhoods, i.e., the closest 
            // neighbors for each vertex, for each time period.
            vector<Vertex> neighbors = V;
            // Remove all vertices that have INFTY makespan.
            neighbors.erase(remove_if(neighbors.begin(), neighbors.end(), 
                [&makespans_i_t](Vertex j) -> bool
                {
                    return makespans_i_t[j] == INFTY;
                }
            ), neighbors.end());
            if (neighbors.empty()) continue; // No neighbors for the vertex.
            
            // Sort the vertices by makespan.
            sort(neighbors.begin(), neighbors.end(), 
                [&makespans_i_t](Vertex u, Vertex v) -> bool
                {
                    return makespans_i_t[u] < makespans_i_t[v];
                }
            );

            // print each vertex and its makespan
            clog << " - Vertex " << i << " in period " << t << " at time " << partitioned_horizon.get_interval(t).left << " makespans: ";
            for (Vertex j: neighbors)
            {
                clog << j;
                if (j == i) clog << " (self)";
                clog << " -> " << makespans_i_t[j];
                if (j != neighbors.back()) clog << ", ";
            }
            clog << endl;
            // Remove the vertex itself
            neighbors.erase(remove(neighbors.begin(), neighbors.end(), i), neighbors.end());

            // Keep only the closest neighbors.
            if (neighbors.size() > nb_neighbors_to_keep)
                neighbors.resize(nb_neighbors_to_keep);
            
            // Store the neighbors.
            EATs_neighborhoods[i][t] = create_bitset<MAX_N>(neighbors);
        }
    }

    // Compare the neighborhoods.
    for (Vertex i: V)
    {
        for (int t = 0; t < nb_horizon_partitions; ++t)
        {
            if (ng_td_neighborhoods_[i][t] != EATs_neighborhoods[i][t])
            {
                clog << "🟢 - Vertex " << i << " in period " << t << " at time " << partitioned_horizon.get_interval(t).left << " neighborhoods differ." << endl;
            }
        }
    }
}

const VertexSet& TDNGNeighborhoods::neighbors(goc::Vertex i, TimeUnit t) const
{
    size_t time_period_index = partitioned_horizon_.interval_index_or_throw(t);
    return ng_td_neighborhoods_[i][time_period_index];
}

} // namespace
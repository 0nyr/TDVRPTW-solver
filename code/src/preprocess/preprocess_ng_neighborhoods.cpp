#include "preprocess/preprocess_ng_neighborhoods.h"
#include "goc/math/math_utils.h"

#include <vector>

using namespace std;
using namespace goc;
using namespace nlohmann;

namespace solver
{
namespace
{
// Partition the time horizon into intervals following the given strategy.
// Precondition: There must be more than one time step.
PartitionedInterval partition_time_horizon(
    const Interval& horizon,
    const vector<Interval>& time_steps,
    TDNGNeighborhoodsTimeStrategy time_strategy
) {
    if (time_strategy == TDNGNeighborhoodsTimeStrategy::TimeStepSpecific)
    {
        // Reuse the time steps directly.
        return PartitionedInterval(time_steps);
    }
    else if (time_strategy == TDNGNeighborhoodsTimeStrategy::PartitionedHorizon)
    {
        // Aggregate the time steps.
        int nb_partitions_of_horizon = fast_log2(time_steps.size()) + 1; // better than just dividing by some value.
        
        vector<double> breakpoints;
        // Add first breakpoint.
        breakpoints.push_back(horizon.left);
        
        // Add the rest of the breakpoints, except the last one.
        double period_duration = (horizon.right - horizon.left) / nb_partitions_of_horizon;
        for (int i = 1; i < nb_partitions_of_horizon - 1; ++i)
        {
            breakpoints.push_back(i*period_duration + horizon.left);
        }
        // Add last breakpoint.
        breakpoints.push_back(horizon.right);

        assert (breakpoints.size() == nb_partitions_of_horizon && "Number of breakpoints must be equal to the number of partitions of the horizon.");
        
        return PartitionedInterval(breakpoints);
    }
    else if (time_strategy == TDNGNeighborhoodsTimeStrategy::RepresentativeTimePeriods)
    {
        // Hard. Dynamically determine the representative time periods for every travel time functions and aggregate them.
        // TODO: Implement this.
        throw runtime_error("Not implemented.");
    }
    else if (time_strategy == TDNGNeighborhoodsTimeStrategy::Static)
    {
        // Only one interval: the horizon.
        return PartitionedInterval(horizon);
    }
    else
    {
        throw runtime_error("Unknown time strategy.");
    }
}
} // anonymous namespace  

void preprocess_ng_neighborhoods(nlohmann::json& instance)
{
    clog << " - NG neighborhoods" << endl;

    // Step 1: Determine time periods to compute the neighborhoods on.
    const Interval horizon = instance["horizon"];
    const vector<Interval> time_steps = instance["time_steps"];
    PartitionedInterval partitioned_horizon = partition_time_horizon(horizon, time_steps, TDNGNeighborhoodsTimeStrategy::TimeStepSpecific);

    Digraph D = instance;
	int n = D.NbVertices();
	auto& V = D.Vertices();
}
} // namespace
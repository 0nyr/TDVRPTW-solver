#include "labeling/ng_neighborhoods.h"
#include "goc/math/math_utils.h"

using namespace std;
using namespace goc;

namespace solver
{
namespace
{
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
    NHPS horizon_partitioning_strategy
):
    partitioned_horizon_(partition_time_horizon(vrp.Horizon(), vrp.TimeSteps(), horizon_partitioning_strategy)),
{

}
} // namespace
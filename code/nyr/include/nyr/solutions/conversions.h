#pragma once

#include <goc/goc.h>

#include "nyr/vrp/instance.h"
#include "nyr/solutions/vrp_solution.h"

namespace nyr
{

VRPSolutionDuration convert_makespan_solution_to_duration(
    const VRPSolutionMakespan& makespan_solution,
    const VRPInstance& vrp
);

VRPSolutionTravelTime convert_makespan_solution_to_travel_time(
    const VRPSolutionMakespan& makespan_solution,
    const VRPInstance& vrp
);

}
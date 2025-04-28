#pragma once

#include <goc/goc.h>

#include "nyr/solutions/objectives.h"
#include "nyr/time/time.h"

namespace nyr
{

class GlobalParams: public goc::Printable
{
public:
    const ProgramClock& pclock; // Program clock to measure time.
    const ObjectiveFunction objective; // global objective function to optimize.
    const nyr::Durex time_limit; // global time limit

    GlobalParams(
        const ProgramClock& pclock,
        ObjectiveFunction objective,
        nyr::Durex time_limit
    ):  
        pclock(pclock),
        objective(objective), 
        time_limit(time_limit) 
    {}

    void Print(std::ostream& os) const override
    {
        os << "Global Parameters:" << std::endl;
        os << "  objective: " << objective << std::endl;
        os << "  time_limit: " << time_limit.count() << " seconds" << std::endl;
    }
};

} // namespace nyr
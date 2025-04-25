#pragma once

#include <goc/goc.h>

#include "nyr/params/constants.h"
#include "nyr/time/time.h"

namespace nyr
{

class GlobalParams: public goc::Printable
{
public:
    const ObjectiveFunction objective; // global objective function to optimize.
    const nyr::Durex time_limit; // global time limit

    GlobalParams(
        ObjectiveFunction objective,
        nyr::Durex time_limit
    ):  
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
#pragma once

#include <vector>
#include <iostream>
#include <concepts>

#include "goc/lib/json.hpp"
#include "goc/print/printable.h"
#include "goc/vrp/vrp_solution.h"

namespace goc
{
// This class stores all the Upper Bound solutions 
// found by a solver at different times.
// The solutions are stored in a vector of tuples
// where the first element is the time since the
// algorithm started, and the second element is the
// solution found.
// The solution type must be serializable to JSON.
template<std::derived_from<Printable> Solution>
class TimedSolutions : public Log
{
public:
    // Adds a new solution to the list.
    void add(Duration time, Solution solution)
    {
        // check that the solution to add is not the same as the last 
        // solution added.
        if (!timed_sols_.empty() && solution == timed_sols_.back().second)
            throw std::runtime_error("The solution to add is the same as the last solution added.");

        timed_sols_.push_back({time, solution});
    }

    // Check if there are any solutions.
    bool empty() const
    {
        return timed_sols_.empty();
    }

    // Returns the solutions.
    const std::vector<std::pair<Duration, Solution>>& get_timed_solutions() const
    {
        return timed_sols_;
    }

    // Returns the last (best) solution found.
    const Solution& last_solution() const
    {
        return timed_sols_.back().second;
    }

    // Serializes the object to JSON.
    // Format: [{"time": time, "solution": solution}, ...]
    nlohmann::json ToJSON() const
    {
        nlohmann::json j;
        std::vector<nlohmann::json> j_sols;
        j_sols.reserve(timed_sols_.size());
        for (auto& [time, solution]: timed_sols_)
        {
            j_sols.push_back({{"time", time}, {"solution", solution}});
        }
        j = j_sols;
        return j;
    }

private:
    // List of solutions with the time they were found, in increasing order of time.
    std::vector<std::pair<Duration, Solution>> timed_sols_;
};

// Type alias for timed VRP solutions.
using TimedVrpSolution = goc::TimedSolutions<goc::VRPSolution>;
} // namespace
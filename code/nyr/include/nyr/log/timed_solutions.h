#pragma once

#include <vector>
#include <iostream>
#include <concepts>

#include <goc/goc.h>
#include "nyr/time/time.h"

namespace nyr
{
// This class stores all the Upper Bound solutions 
// found by a solver at different times.
// The solutions are stored in a vector of tuples
// where the first element is the time since the
// algorithm started, and the second element is the
// solution found.
// The solution type must be serializable to JSON.
template<std::derived_from<goc::AbstractSolution> Solution>
class TimedSolutions : public goc::Log
{
public:
    const ProgramClock& pclock; // Program clock to measure time.

    TimedSolutions(const nyr::ProgramClock& pclock)
        : pclock(pclock)
    {}

    /**
     * Adds a new solution to the list of solutions.
     */
    inline void add(Durex time, Solution solution)
    {
        timed_sols_.push_back({time, solution});
    }

    /**
     * Try to add a new solution to the list of solutions.
     * Only adds the solution if it is not the same as the last
     * solution added, and if its value is smaller than the 
     * last solution added (minimization problem). 
     */
    void try_add(Solution solution)
    {
        // check that the solution to add is not the same as the last 
        // solution added, and if its value is smaller than the last solution added.
        if (
            !timed_sols_.empty() && 
            (solution == timed_sols_.back().second ||
            solution.value >= timed_sols_.back().second.value)
        )
            return;

        // Get the time since the program started.
        add(pclock.elapsed(), solution);
    }

    // Check if there are any solutions.
    bool empty() const
    {
        return timed_sols_.empty();
    }

    // Returns the solutions.
    const std::vector<std::pair<Durex, Solution>>& get_timed_solutions() const
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
    std::vector<std::pair<Durex, Solution>> timed_sols_;
};

// Type alias for timed VRP solutions.
using TimedVrpSolution = nyr::TimedSolutions<goc::VRPSolution>;
} // namespace
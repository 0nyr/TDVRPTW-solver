#pragma once

#include <vector>
#include <iostream>
#include <concepts>
#include <ranges>
#include <string>
#include <memory>

#include <goc/goc.h>
#include "nyr/time/time.h"
#include "nyr/solutions/vrp_solution.h"
#include "nyr/solutions/route.h"
#include "nyr/solutions/objectives.h"

namespace nyr
{

class AbstractSolutionRecord: public goc::Log
{
public:
    virtual ~AbstractSolutionRecord() = default;

    // Add a solution
    virtual void add(nyr::Durex time, std::unique_ptr<AbstractSolution> sol, const std::string& origin) = 0;

    // Try to add (only if better)
    virtual void try_add(std::unique_ptr<AbstractSolution> sol, const std::string& origin) = 0;

    // Last solution value
    virtual double last_solution_value() const = 0;

    // Empty?
    virtual bool empty() const = 0;

    // Export all solutions to JSON
    virtual nlohmann::json ToJSON() const = 0;
};

// A record entry for a solution found by the solver.
// Contains useful information about the solution:
// - The time at which the solution was found.
// - The solution itself.
// - The origin or source of the solution (e.g. "GMH1", "BPCA", ...).
template<std::derived_from<AbstractSolution> Solution>
struct AnnotatedSolution
{
    Durex time;         // Time since the algorithm started.
    Solution solution;  // The solution found.
    std::string origin; // The origin or source of the solution.
};

// This class stores all the Upper Bound solutions 
// found by a solver at increasing times and quality.
// It is used to log the solutions found by the solver.
// The solution type must be serializable to JSON.
template<std::derived_from<AbstractSolution> Solution>
class SolutionRecord: public AbstractSolutionRecord
{
public:
    const ProgramClock& pclock; // Program clock to measure time.

    SolutionRecord(const nyr::ProgramClock& pclock)
        : pclock(pclock)
    {}

    /**
     * Adds a new solution to the list of solutions.
     */
    inline void add(Durex time, Solution solution, std::string origin)
    {
        sol_records_.push_back({time, solution, origin});

        std::clog << "✨[" << origin << "]> Solution: "
            << "nb routes: " << solution.routes.size()
            << ", value: " << solution.value
            << " - routes: " << solution.routes 
            << std::endl;
    }

    /**
     * Try to add a new solution to the list of solutions.
     * Only adds the solution if it is not the same as the last
     * solution added, and if its value is smaller than the 
     * last solution added (minimization problem). 
     */
    void try_add(Solution solution, std::string origin)
    {
        // check that the solution to add is not the same as the last 
        // solution added, and if its value is smaller than the last solution added.
        if (
            !sol_records_.empty() && 
            (solution == sol_records_.back().solution ||
            solution.value >= sol_records_.back().solution.value)
        )
            return;

        // Get the time since the program started.
        add(pclock.elapsed(), solution, origin);
    }

    // Check if there are any solutions.
    bool empty() const
    {
        return sol_records_.empty();
    }

    // Returns the solutions.
    const std::vector<AnnotatedSolution<Solution>>& get_timed_solutions() const
    {
        return sol_records_;
    }

    // Returns a range to iterate over the solutions directly.
    auto solutions() const
    {
        return sol_records_ | std::views::transform([](const auto& annotated_sol) -> const Solution& {
            return annotated_sol.solution;
        });
    }

    // Returns the last (best) solution found.
    const Solution& last_solution() const
    {
        return sol_records_.back().solution;
    }

    // Returns the last (best) solution value found
    // or INFTY if no solution was found.
    double last_solution_value() const
    {
        return sol_records_.empty() ? goc::INFTY : sol_records_.back().solution.value;
    }

    // Serializes the object to JSON.
    // Format: [{"time": time, "solution": solution}, ...]
    nlohmann::json ToJSON() const
    {
        nlohmann::json j;
        std::vector<nlohmann::json> j_sols;
        j_sols.reserve(sol_records_.size());
        for (auto& annotated_sol: sol_records_)
        {
            j_sols.push_back({
                {"time", annotated_sol.time},
                {"origin", annotated_sol.origin},
                {"solution", annotated_sol.solution}
            });
        }
        j = j_sols;
        return j;
    }

private:
    // List of solutions with the time they were found, in increasing order of time.
    std::vector<AnnotatedSolution<Solution>> sol_records_;
};

std::unique_ptr<AbstractSolutionRecord> create_solution_record(
    const nyr::ProgramClock& pclock, 
    nyr::ObjectiveFunction obj_func
)
{
    switch (obj_func)
    {
    case nyr::ObjectiveFunction::Makespan:
        return std::make_unique<nyr::SolutionRecord<VRPSolutionMakespan>>(pclock);
    case nyr::ObjectiveFunction::Duration:
        return std::make_unique<nyr::SolutionRecord<VRPSolutionDuration>>(pclock);
    case nyr::ObjectiveFunction::TravelTime:
        return std::make_unique<nyr::SolutionRecord<VRPSolutionTravelTime>>(pclock);
    default:
        throw std::runtime_error("Unsupported objective function.");
    }
}

} // namespace
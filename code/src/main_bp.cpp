#include <iostream>
#include <vector>
#include <climits>
#include <chrono>
#include <cmath>
#include <string>
#include <memory>

#include <goc/goc.h>
#include <nyr/nyr.h>

#include "preprocess/load_igp.h"
#include "preprocess/preprocess_validity.h"
#include "heuristics/greedy_makespan.h"
#include "heuristics/aco.h"

using namespace std;
using namespace goc;
using namespace nyr;
using namespace nlohmann;
using namespace solver;

/**
 * @brief Solve the TDVRPTW problem using the makespan objective.
 */
SolutionRecord<VRPSolutionMakespan> solve_makespan(
    const VRPInstance& vrp,
    const ProgramClock& pclock,
    const nyr::GlobalParams& gparams,
    const AntColonyParams& aco_options
) {
    SolutionRecord<VRPSolutionMakespan> solution_record(pclock);

    // Initialization heuristics.
    if (gparams.initialization_heuristics) {
        clog << "Initialization heuristics..." << endl;
		gmh1(solution_record, vrp);

        auto end_status = aco(solution_record, vrp, aco_options);
		if (end_status == ACOStatus::TimeLimitReached) return solution_record;
    }

    return solution_record;
}

int main(int argc, char** argv)
{
    // Initialization
	const nyr::ProgramClock pclock; // start the program clock.
    json output; // STDOUT output will go into this JSON.

	try
	{
        // Step 1: Load instance & experiment parameters.
        if (argc > 1) {
            simulate_runner_input(
                "instances/for_testing", 
                "Ari1-n=17-00bf7cc8d47a156268eb6fa52fd053cda8d5b184.json", 
                "experiments/bp_test.json", 
                "BP-CUTS"
            );
        }
		
        json experiment, instance, solutions;
        cin >> experiment >> instance >> solutions;
        // clog << "Experiment: " << experiment << endl;
        // clog << "Instance: " << instance << endl;
        // clog << "Solutions: " << solutions << endl;
        load_igp(instance);
        preprocess_validity(instance);
        VRPInstance vrp = instance;

        // Parse experiment.
        Duration time_limit = value_or_default(experiment, "time_limit", 2.0_hr);
        
        const nyr::GlobalParams gparams = nyr::GlobalParams({
            pclock,
            enum_value_or_default<nyr::ObjectiveFunction>(experiment, "objective", nyr::ObjectiveFunction::Duration),
            nyr::Durex(time_limit.Amount(goc::DurationUnit::Seconds)),
            value_or_default(experiment, "initialization_heuristics", true)
        });
        const nyr::BCPParams bcp_params = nyr::BCPParams(
            pclock,
            value_or_default(experiment, "cut_limit", 100),
            value_or_default(experiment, "node_limit", INT_MAX)
        );
        const double ratio_nb_neighbors = 0.09;
	    const int ng_nb_neighbors = value_or_default(experiment, "ng_nb_neighbors", std::round(ratio_nb_neighbors * (double)instance["nb_vertices"]));
	    const int ng_max_neighbors = max((int)((double)instance["nb_vertices"] / 2.0), ng_nb_neighbors);
        const nyr::BidirectionalLabelingParams bl_params = nyr::BidirectionalLabelingParams(
            pclock,
            value_or_default(experiment, "partial", true),
            value_or_default(experiment, "limited_extension", true),
            value_or_default(experiment, "lazy_extension", true),
            value_or_default(experiment, "unreachable_strengthened", true),
            value_or_default(experiment, "sort_by_cost", true),
            value_or_default(experiment, "symmetric", false),
            value_or_default(experiment, "iterative_merge", true),
            value_or_default(experiment, "lal_heuristic_cost", true),
            value_or_default(experiment, "lal_heuristic_elementarity", true),
            value_or_default(experiment, "lal_heuristic_ng_routes", false),
            value_or_default(experiment, "lal_exact_labeling", true),
            ratio_nb_neighbors,
            ng_nb_neighbors,
            ng_max_neighbors
        );
        const AntColonyParams aco_options = AntColonyParams(
            gparams,
            value_or_default(experiment, "aco_max_nb_iterations", 1000), 
            value_or_default(experiment, "aco_max_no_improvement", 500),
            value_or_default(experiment, "aco_nb_ants", 2),
            value_or_default(experiment, "aco_alpha", 1),
            value_or_default(experiment, "aco_beta", 2),
            value_or_default(experiment, "aco_rho", 0.05),
            value_or_default(experiment, "aco_tau_min", 0.000001),
            value_or_default(experiment, "aco_tau_0", 1.0),
            value_or_default(experiment, "aco_tau_max", 10.0),
            value_or_default(experiment, "aco_delta_pheromone_threshold", 0.000001)
        );

        #ifndef NDEBUG
        clog << "DEBUG MODE ACTIVE" << endl;
        #endif

        // Show instance details.
        clog << "Experiment: " << experiment["name"] << endl;
        clog << "Instance: " << instance["instance_basename"] << " - " << value_or_default(instance, "instance_filename", "(filename missing)") << endl;
        clog << "Benchmark: " << instance["benchmark_basename"] << endl;
        clog << "Nb vertices: " << instance["nb_vertices"] << endl;
        gparams.Print(clog);
        bcp_params.Print(clog);
        bl_params.Print(clog);
        aco_options.Print(clog);

        clog << "Loading & Preprocessing time: " << pclock.elapsed() << endl;

        // Step 2: Solve the problem depending on the objective function.
        switch (gparams.objective)
        {
            case nyr::ObjectiveFunction::Makespan: {
                auto rec = solve_makespan(vrp, pclock, gparams, aco_options);
                print_last_solution(rec, gparams.objective);
                output["timed_solutions"] = rec;
                break;
            }
            // case nyr::ObjectiveFunction::Duration: {
            //     rec = solve_duration(argc, argv, pclock, output);
            //     break;
            // }
            default: {
                throw_invalid_objective_function(gparams.objective);
                break;
            }
        }

        // Send JSON output to cout.
        cout << output << endl;

        clog << "Full run time: " << pclock.elapsed() << endl;

	}
	catch (std::bad_alloc& e)
	{
		return 3;
	}
	return 0;
}
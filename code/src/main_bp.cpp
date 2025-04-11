//
// Created by Gonzalo Lera Romero.
// Grupo de Optimizacion Combinatoria (GOC).
// Departamento de Computacion - Universidad de Buenos Aires.
//

#include <iostream>
#include <vector>
#include <climits>
#include <chrono>

#include <goc/goc.h>
#include <nyr/nyr.h>

#include "instance/vrp_instance.h"
#include "instance/load_igp.h"
#include "bcp/bcp.h"
#include "bcp/spf.h"
#include "bcp/pricing_problem.h"
#include "labeling/bidirectional_labeling.h"
#include "goc/log/timed_solutions.h"
#include "goc/math/math_utils.h"
#include "preprocess/preprocess_validity.h"
#include "labeling/ng_neighborhoods.h"
#include "labeling/labeling_level.h"

#include "heuristics/greedy_makespan.h"

using namespace std;
using namespace goc;
using namespace nlohmann;
using namespace solver;

// Returns: the cost of a path.
// NOTE: Calculated as the duration of the path minus the sum of 
// the profits of the vertices minus the sum of the duals of the cuts.
double path_cost(
	const VRPInstance& vrp, 
	PricingProblem pp, 
	GraphPath path
) {
	VertexSet column;
	for (Vertex i: path) column.set(i);
	return vrp.BestDurationRoute(path).duration 
		- sum<Vertex>(path, [&] (Vertex v) { return pp.P[v]; })
		- sum<int>(range(0, pp.S.size()), [&] (int i) { return intersection(pp.S[i], column).count() >= 2 ? pp.sigma[i] : 0.0; });
}

int main(int argc, char** argv)
{
	try
	{
		auto program_start_time = nyr::Clock::now();

		json output; // STDOUT output will go into this JSON.

		if (argc > 1) 
			simulate_runner_input(
				"instances/for_testing", 
				"Ari1-n=17-00bf7cc8d47a156268eb6fa52fd053cda8d5b184.json", 
				"experiments/bp_test.json", 
				"BP-CUTS"
			);

		json experiment, instance, solutions;
		cin >> experiment >> instance >> solutions;
		// clog << "Experiment: " << experiment << endl;
		// clog << "Instance: " << instance << endl;
		// clog << "Solutions: " << solutions << endl;
		load_igp(instance);

		// Parse experiment.
		Duration time_limit = value_or_default(experiment, "time_limit", 2.0_hr);
		int cut_limit = value_or_default(experiment, "cut_limit", 100);
		int node_limit = value_or_default(experiment, "node_limit", INT_MAX);
		bool partial = value_or_default(experiment, "partial", true);
		bool limited_extension = value_or_default(experiment, "limited_extension", true);
		bool lazy_extension = value_or_default(experiment, "lazy_extension", true);
		bool unreachable_strengthened = value_or_default(experiment, "unreachable_strengthened", true);
		bool sort_by_cost = value_or_default(experiment, "sort_by_cost", true);
		bool symmetric = value_or_default(experiment, "symmetric", false);
		bool iterative_merge = value_or_default(experiment, "iterative_merge", true);
		
		// Labeling Algorithm levels
		const bool lal_heuristic_cost = value_or_default(experiment, "lal_heuristic_cost", true);
		const bool lal_heuristic_elementarity = value_or_default(experiment, "lal_heuristic_elementarity", true);
		const bool lal_heuristic_ng_routes = value_or_default(experiment, "lal_heuristic_ng_routes", false);
		const bool lal_exact_labeling = value_or_default(experiment, "lal_exact_labeling", true);

		//int ng_nb_neighbors = double(fast_log2((uint32_t)instance["nb_vertices"])) * 1.7;
		const double ratio_nb_neighbors = 0.09;
		const int ng_nb_neighbors = value_or_default(experiment, "ng_nb_neighbors", std::round(ratio_nb_neighbors * (double)instance["nb_vertices"]));
		const int ng_max_neighbors = max((int)((double)instance["nb_vertices"] / 2.0), ng_nb_neighbors);

		// Show instance details.
		clog << "Experiment: " << experiment["name"] << endl;
		clog << "Instance: " << instance["instance_basename"] << " - " << value_or_default(instance, "instance_filename", "(filename missing)") << endl;
		clog << "Benchmark: " << instance["benchmark_basename"] << endl;
		clog << "Nb vertices: " << instance["nb_vertices"] << endl;

		// Show experiment details.
		clog << "Time limit: " << time_limit << "s." << endl;
		clog << "Cut limit: " << cut_limit << endl;
		clog << "Node limit: " << node_limit << endl;
		clog << "Partial: " << partial << endl;
		clog << "Limited extension: " << limited_extension << endl;
		clog << "Lazy extension: " << lazy_extension << endl;
		clog << "Unreachable strengthened: " << unreachable_strengthened << endl;
		clog << "Sort by cost: " << sort_by_cost << endl;
		clog << "Symmetric: " << symmetric << endl;
		clog << "Iterative merge: " << iterative_merge << endl;
		clog << "LAL: Heuristic cost: " << lal_heuristic_cost << endl;
		clog << "LAL: Heuristic elementarity: " << lal_heuristic_elementarity << endl;
		clog << "LAL: Heuristic NG routes: " << lal_heuristic_ng_routes << endl;
		clog << "LAL: Exact labeling: " << lal_exact_labeling << endl;
		if (lal_heuristic_ng_routes)
		{
			clog << "NG nb neighbors: " << ng_nb_neighbors << endl;
			clog << "NG max neighbors: " << ng_max_neighbors << endl;
		}

		preprocess_validity(instance);
		// preprocess_ng_neighborhoods(
		// 	instance,
		// 	TDNGNeighborhoodsTimeStrategy::TimeStepSpecific,
		// 	ng_nb_neighbors
		// ); 
		clog << "Preprocessing time: " <<  nyr::seconds_since(program_start_time) << endl;

		// Parse instance.
		VRPInstance vrp = instance;

		ghm1_duration(vrp);
		// Log solve time
		clog << "Full run time: " << nyr::seconds_since(program_start_time) << endl;
		return 0;

		// Run BCP.
		clog << "Running BCP algorithm..." << endl;

		// Create SPF and add initial routes (o, i, d).
		SPF spf(vrp.D.NbVertices());
		for (Vertex i: exclude(vrp.D.Vertices(), {vrp.o, vrp.d}))
			spf.AddRoute(vrp.BestDurationRoute({vrp.o, i, vrp.d}));

		// The Branch-Cut-Price algorithm to solve the VRP.
		BCP bcp(vrp.D, &spf);
		bcp.time_limit = time_limit;
		bcp.cut_limit = cut_limit;
		bcp.node_limit = node_limit;
		
		// The labeling algorithm which is used in the CG solver of the BCP for the pricing problem.
		std::optional<TDNGRoutesParams> ng_routes_params = [&]() -> std::optional<TDNGRoutesParams> {
			// NG-Routes will be use, so we need to create the neighborhoods.
			if (lal_heuristic_ng_routes) {
				return TDNGRoutesParams(
					ng_nb_neighbors, 
					ng_max_neighbors,
					partition_time_horizon(
						{0, vrp.T},
						vrp.time_steps,
						NHPS::TimeStepSpecific
					)
				);
			} else {
				// NG-routes not used. Save compute time/memory.
				return std::nullopt;
			}
		}();

		BidirectionalLabeling lbl(
			vrp,
			ng_routes_params
		);
		lbl.solution_limit = 3000;
		lbl.closing_state = !iterative_merge;
		lbl.partial = partial;
		lbl.limited_extension = limited_extension;
		lbl.lazy_extension = lazy_extension;
		lbl.unreachable_strengthened = unreachable_strengthened;
		lbl.sort_by_cost = sort_by_cost;
		lbl.symmetric = symmetric;

		// Define the levels to try in order
		std::vector<std::pair<LabelingLevel, std::string>> levels_to_try;
		if (lal_heuristic_cost) levels_to_try.emplace_back(LabelingLevel::HeuristicCost, "Heuristic Cost");
		if (lal_heuristic_elementarity) levels_to_try.emplace_back(LabelingLevel::HeuristicElementarity, "Heuristic Elementarity");
		if (lal_heuristic_ng_routes) levels_to_try.emplace_back(LabelingLevel::HeuristicNG, "Heuristic NG Routes");
		if (lal_exact_labeling) levels_to_try.emplace_back(LabelingLevel::Exact, "Exact");
		if (levels_to_try.empty())
		{
			clog << "No labeling levels enabled." << endl;
			return 0;
		}

		bcp.pricing_solver = [&](
			const PricingProblem& pricing_problem, 
			int node_number, 
			Duration tlimit, 
			CGExecutionLog* cg_execution_log
		) {
			Stopwatch iteration_rolex(true);
			std::vector<Route> R;

			for (const auto& [level, level_name] : levels_to_try) {
				lbl.time_limit = tlimit - iteration_rolex.Peek();
				auto lbl_log = lbl.Run(pricing_problem, &R, level);
				
				// Add iteration log.
				cg_execution_log->iterations->push_back(lbl_log);
				cg_execution_log->iterations->back()["iteration_name"] = level_name;

				// Update merge_start and closing_state.
				lbl.closing_state |= level == LabelingLevel::Exact && lbl_log.status == BLBStatus::Finished;
				lbl.merge_start = (lbl.merge_start + lbl_log.forward_log->processed_count) / 2;

				if (!R.empty()) break;
			}

			if (!R.empty())
			{
				// Add negative reduced cost routes.
				for (auto& route : R) spf.AddRoute(route);
			}
			else
			{
				// If no routes were found, reset the labeling algorithm.
				lbl.closing_state = false;
				lbl.merge_start = 0;
			}
		};


		TimedSolutions<VRPSolution> timed_solutions({});
		auto log = bcp.Run(timed_solutions);

		output["Exact"] = log;
		output["timed_solutions"] = timed_solutions;

		clog << "Time: " << log.time << endl;
		clog << "#Nodes: " << log.nodes_closed << endl;
		clog << "Status: " << log.status << endl;
		if (timed_solutions.empty())
			clog << "No solution found." << endl;
		else
		{
			auto& best_solution = timed_solutions.last_solution();
			clog << "Best solution:" << endl;
			clog << "\tValue: " << best_solution.value << endl;
			clog << "\tRoutes:" << endl;
			for (auto& r: best_solution.routes) clog << "\t\t" << r << endl;
		}

		// Send JSON output to cout.
		cout << output << endl;
	}
	catch (std::bad_alloc& e)
	{
		return 3;
	}
	return 0;
}
//
// Created by Gonzalo Lera Romero.
// Grupo de Optimizacion Combinatoria (GOC).
// Departamento de Computacion - Universidad de Buenos Aires.
//

#include <iostream>
#include <vector>
#include <goc/goc.h>

#include "instance/vrp_instance.h"
#include "preprocess/preprocess_travel_times.h"
#include "preprocess/preprocess_capacity.h"
#include "preprocess/preprocess_time_windows.h"
#include "preprocess/preprocess_service_waiting.h"
#include "preprocess/preprocess_triangle_depot.h"

#include "labeling/bidirectional_labeling.h"

using namespace std;
using namespace goc;
using namespace nlohmann;
using namespace solver;

namespace
{
// Returns: the sum of all profits in p.
ProfitUnit path_profit(const GraphPath& p, const vector<ProfitUnit>& profits)
{
	return sum<Vertex>(p, [&] (Vertex v) { return profits[v]; });
}
}

int main(int argc, char** argv)
{
	try
	{
		json output; // STDOUT output will go into this JSON.

		if (argc > 1)
			simulate_runner_input("instances/networks_2019b", "RC204_25_a", "experiments/pricing.json", "Basic");

		json experiment, instance, solutions;
		cin >> experiment >> instance >> solutions;

		// Parse experiment.
		Duration time_limit = value_or_default(experiment, "time_limit", 2.0_hr);
		bool correcting = value_or_default(experiment, "correcting", false);
		bool partial = value_or_default(experiment, "partial", true);
		bool limited_extension = value_or_default(experiment, "limited_extension", true);
		bool lazy_extension = value_or_default(experiment, "lazy_extension", true);
		bool unreachable_strengthened = value_or_default(experiment, "unreachable_strengthened", true);
		bool sort_by_cost = value_or_default(experiment, "sort_by_cost", true);
		bool symmetric = value_or_default(experiment, "symmetric", false);

		// Show experiment details.
		clog << "Time limit: " << time_limit << "s." << endl;
		clog << "Correcting: " << correcting << endl;
		clog << "Partial: " << partial << endl;
		clog << "Limited extension: " << limited_extension << endl;
		clog << "Lazy extension: " << lazy_extension << endl;
		clog << "Unreachable strengthened: " << unreachable_strengthened << endl;
		clog << "Sort by cost: " << sort_by_cost << endl;
		clog << "Symmetric: " << symmetric << endl;

		// Preprocess instance JSON.
		clog << "Preprocessing..." << endl;
		preprocess_capacity(instance);
		preprocess_travel_times(instance);
		preprocess_service_waiting(instance);
		preprocess_time_windows(instance);
		preprocess_triangle_depot(instance);

		// Parse instance.
		VRPInstance vrp = instance;

		// Read pricing problem.
		PricingProblem pp;
		pp.P = vector<ProfitUnit>(instance["profits"].begin(), instance["profits"].end());

		clog << "Running pricing algorithm..." << endl;
		BidirectionalLabeling lbl(vrp);
		lbl.screen_output = &clog;
		lbl.time_limit = time_limit;
		lbl.correcting = correcting;
		lbl.partial = partial;
		lbl.limited_extension = limited_extension;
		lbl.lazy_extension = lazy_extension;
		lbl.unreachable_strengthened = unreachable_strengthened;
		lbl.sort_by_cost = sort_by_cost;
		lbl.symmetric = symmetric;
		vector<Route> R;
		BLBExecutionLog log = lbl.Run(pp, &R);

		clog << "Total time: " << log.time << endl;
		// Get best route.
		if (!R.empty())
		{
			Route best = R[0];
			for (auto &r: R)
				if (r.duration - path_profit(r.path, pp.P) < best.duration - path_profit(best.path, pp.P))
					best = r;
			clog << "Best solution: " << endl;
			clog << "\tpath: " << best.path << endl;
			clog << "\tt0: " << best.t0 << endl;
			clog << "\tduration: " << best.duration << endl;
			clog << "\tcost: " << best.duration - path_profit(best.path, pp.P) << endl;
			output["Best solution"] = VRPSolution(best.duration - path_profit(best.path, pp.P), {best});
		}
		output["Exact"] = log;

		// Send JSON output to cout.
		cout << output << endl;
	}
	catch (std::bad_alloc& e)
	{
		return 3;
	}
	return 0;
}
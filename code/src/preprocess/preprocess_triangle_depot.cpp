//
// Created by Gonzalo Lera Romero.
// Grupo de Optimizacion Combinatoria (GOC).
// Departamento de Computacion - Universidad de Buenos Aires.
//

#include "preprocess/preprocess_triangle_depot.h"

#include "vrp_instance.h"

using namespace std;
using namespace goc;
using namespace nlohmann;

namespace solver
{
namespace
{
// Removes the arc ij from the instance.
void remove_arc(Vertex i, Vertex j, json& instance)
{
	if (instance["digraph"]["arcs"][i][j] == 0) return;
	instance["digraph"]["arcs"][i][j] = 0;
	int arc_count = instance["digraph"]["arc_count"];
	instance["digraph"]["arc_count"] = arc_count - 1;
	if (has_key(instance, "travel_times")) instance["travel_times"][i][j] = vector<json>({});
}
}

void preprocess_triangle_depot(json& instance)
{
	VRPInstance vrp = instance;
	Vertex o = vrp.o, d = vrp.d;
	for (Vertex i: vrp.D.Vertices())
	{
		for (Vertex j: vrp.D.Successors(i))
		{
			if (i == o || j == d) continue;
			TimeUnit b_i = max(vrp.tw[i]), a_j = min(vrp.tw[j]);
			double t0_ij = vrp.TravelTime({i, d}, b_i) + vrp.TravelTime({o, j}, vrp.ArrivalTime({i, d}, b_i));
			if (epsilon_smaller_equal(t0_ij, a_j - b_i))
				remove_arc(i, j, instance);
		}
	}
}
} // namespace
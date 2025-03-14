#include "preprocess/preprocess_utils.h"

using namespace std;
using namespace goc;
using namespace nlohmann;

namespace solver
{
// Removes the arc ij from the instance.
void remove_arc(
    json& instance,
    Vertex i, 
    Vertex j
) {
	if (instance["arcs"][i][j] == 0) return;
	
	instance["arcs"][i][j] = 0;
	int arc_count = instance["arc_count"];
	instance["arc_count"] = arc_count - 1;

	// Remove arc from the digraph.
	if (has_key(instance, "travel_times")) instance["travel_times"][i][j] = vector<json>({});

}
}
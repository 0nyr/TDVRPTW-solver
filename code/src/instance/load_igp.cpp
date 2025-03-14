#include "instance/load_igp.h"

#include "preprocess/preprocess_travel_times.h"
#include "preprocess/preprocess_capacity.h"
#include "preprocess/preprocess_time_windows.h"
#include "preprocess/preprocess_service_waiting.h"
#include "preprocess/preprocess_triangle_depot.h"

using namespace std;
using namespace goc;
using namespace nlohmann;

const vector<string>required_keys = {
    "nb_vertices", 
    "start_depot", 
    "end_depot", 
    "horizon", 
    "time_windows", 
    "vehicle_capacity", 
    "demands"
};

namespace solver
{
namespace
{
void check_required_keys(
    nlohmann::json& instance,
    const vector<string>& required_keys
) {
    for (const string& key: required_keys)
        if (!has_key(instance, key))
            throw runtime_error("The JSON instance is missing the key: " + key);
}
}

void load_igp(nlohmann::json& instance)
{
    clog << "Loading..." << endl;
    check_required_keys(instance, required_keys);

    // int n = j["nb_vertices"];
    // Digraph D = j;
    // Vertex o = j["start_depot"];
	// Vertex d = j["end_depot"];
	// TimeUnit T = j["horizon"][1];
    // vector<goc::Interval> tw = vector<Interval>(j["time_windows"].begin(), j["time_windows"].end());
    // CapacityUnit Q = j["vehicle_capacity"];
    // vector<CapacityUnit> q = vector<CapacityUnit>(j["demands"].begin(), j["demands"].end());


    clog << "Preprocessing..." << endl;
    preprocess_capacity(instance);
    preprocess_travel_times(instance);
    preprocess_service_waiting(instance);
    preprocess_time_windows(instance);
    preprocess_triangle_depot(instance);
}
} // namespace
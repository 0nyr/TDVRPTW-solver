#include "instance/load_igp.h"

#include "preprocess/preprocess_travel_times.h"
#include "preprocess/preprocess_capacity.h"
#include "preprocess/preprocess_time_windows.h"
#include "preprocess/preprocess_service_waiting.h"
#include "preprocess/preprocess_triangle_depot.h"

using namespace std;
using namespace goc;
using namespace nlohmann;

const vector<string>COMMON_REQUIRED_KEYS = {
    "instance_basename",
    "benchmark_basename",
    "problem_type",
    "nb_vertices",
    "vehicle_capacity",
    "demands",
    "time_windows",
    "service_times",
    "start_depot",
    "end_depot",
    "nb_vehicles",
    "horizon",
    "arc_count",
    "arcs",   
};
const vector<string>SOLONOM1987_REQUIRED_KEYS = {
    "coordinates",
};
const vector<string>DABIA_REQUIRED_KEYS = {
    "nb_speeds",
    "speeds",
    "zones",
    "nb_time_steps",
    "time_steps",
    "distances",
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

void load_solomon1987(nlohmann::json& instance)
{
    check_required_keys(instance, SOLONOM1987_REQUIRED_KEYS);
    
    clog << "Preprocessing..." << endl;
    preprocess_capacity(instance);
    preprocess_constant_travel_times(instance);
    preprocess_service_waiting(instance);
    preprocess_time_windows(instance);
    preprocess_triangle_depot(instance);
}

void load_dabia2013(nlohmann::json& instance)
{
    // Dabia2013 is based on Solomon1987.
    check_required_keys(instance, SOLONOM1987_REQUIRED_KEYS);
    check_required_keys(instance, DABIA_REQUIRED_KEYS);
    
    clog << "Preprocessing..." << endl;
    preprocess_capacity(instance);
    preprocess_travel_times(instance);
    preprocess_service_waiting(instance);
    preprocess_time_windows(instance);
    preprocess_triangle_depot(instance);
}
} // anonymous namespace

void load_igp(nlohmann::json& instance)
{
    clog << "Loading..." << endl;
    check_required_keys(instance, COMMON_REQUIRED_KEYS);
    
    string benchmark_basename = instance["benchmark_basename"];
    if (benchmark_basename == "Dabia2013")
        load_dabia2013(instance);
    else if (benchmark_basename == "Solomon1987")
        load_solomon1987(instance);
    else
        throw runtime_error("The benchmark_basename is not supported: " + benchmark_basename);

    // int n = j["nb_vertices"];
    // Digraph D = j;
    // Vertex o = j["start_depot"];
	// Vertex d = j["end_depot"];
	// TimeUnit T = j["horizon"][1];
    // vector<goc::Interval> tw = vector<Interval>(j["time_windows"].begin(), j["time_windows"].end());
    // CapacityUnit Q = j["vehicle_capacity"];
    // vector<CapacityUnit> q = vector<CapacityUnit>(j["demands"].begin(), j["demands"].end());
}
} // namespace
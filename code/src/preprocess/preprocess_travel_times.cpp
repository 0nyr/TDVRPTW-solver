//
// Created by Gonzalo Lera Romero.
// Grupo de Optimizacion Combinatoria (GOC).
// Departamento de Computacion - Universidad de Buenos Aires.
//

#include "preprocess/preprocess_travel_times.h"

using namespace std;
using namespace goc;
using namespace nlohmann;

namespace solver
{
namespace
{
// Calculates the time to depart to traverse arc e arriving at tf.
// Returns: INFTY if it is infeasible to depart inside the horizon.
double departing_time(const json& instance, Arc e, double tf)
{
	int c = instance["zones"][e.tail][e.head]; // cluster of arc e.
	vector<Interval> T = instance["time_steps"]; // T[k] = speed zone k.
	vector<double> speed = instance["speeds"][c]; // speed[k] = speed of traversing e in speed zone k.
	double d = instance["distances"][e.tail][e.head]; // distance of arc e.
	double t = tf;
	for (int k = (int)T.size()-1; k >= 0; --k)
	{
		if (epsilon_equal(d, 0.0)) break;
		if (epsilon_bigger(T[k].left, tf)) continue;
		double remaining_time_in_k = min(T[k].right, tf) - T[k].left;
		double time_to_complete_d_in_k = d / speed[k];
		double time_in_k = min(remaining_time_in_k, time_to_complete_d_in_k);
		t -= time_in_k;
		d -= time_in_k * speed[k];
	}
	if (epsilon_bigger(d, 0.0)) return INFTY;
	return t;
}


// Calculates the travel time to traverse arc e departing at t0.
// Returns: INFTY if it is infeasible to arrive inside the horizon.
double travel_time(const json& instance, Arc e, double t0)
{
	int c = instance["zones"][e.tail][e.head]; // cluster of arc e.
	vector<Interval> T = instance["time_steps"]; // T[k] = speed zone k.
	vector<double> speed = instance["speeds"][c]; // speed[k] = speed of traversing e in speed zone k.
	double d = instance["distances"][e.tail][e.head]; // distance of arc e.
	double t = t0;
	for (int k = 0; k < T.size(); ++k)
	{
		if (epsilon_equal(d, 0.0)) break;
		if (epsilon_smaller(T[k].right, t0)) continue;
		double remaining_time_in_k = T[k].right - max(T[k].left, t0);
		double time_to_complete_d_in_k = d / speed[k];
		double time_in_k = min(remaining_time_in_k, time_to_complete_d_in_k);
		t += time_in_k;
		d -= time_in_k * speed[k];
	}
	if (epsilon_bigger(d, 0.0)) return INFTY;
	return t-t0;
}

// Returns the time when we arrive at the end of arc e if departing at t0.
double ready_time(const json& instance, Arc e, double t0)
{
	double tt = travel_time(instance, e, t0);
	return tt == INFTY ? tt : t0 + tt;
}

// Precondition: no speeds are 0.
PWLFunction compute_igp_travel_time_function(const json& instance, Arc e)
{
	// Calculate speed breakpoints.
	vector<Interval> speed_zones = instance["time_steps"];
	vector<double> speed_breakpoints;
	for (auto& z: speed_zones) speed_breakpoints.push_back(z.left);
	speed_breakpoints.push_back(speed_zones.back().right);
	
	// Travel time breakpoints are two sets
	// 	- B1: speed breakpoints which are feasible to depart
	// 	- B2: times t such that we arrive to head(e) at a speed breakpoint.
	vector<double> B1;
	for (double t: speed_breakpoints)
		if (travel_time(instance, e, t) != INFTY)
			B1.push_back(t);
		
	vector<double> B2;
	for (double t: speed_breakpoints)
		if (departing_time(instance, e, t) != INFTY)
			B2.push_back(departing_time(instance, e, t));
	
	// Merge breakpoints in order in a set B.
	vector<double> B(B1.size()+B2.size());
	merge(B1.begin(), B1.end(), B2.begin(), B2.end(), B.begin());
	
	// Remove duplicates from B.
	B.resize(distance(B.begin(), unique(B.begin(), B.end())));
	
	// Calculate travel times for each t \in B.
	vector<double> T;
	for (double t: B) T.push_back(travel_time(instance, e, t));
	
	// Create travel time function.
	PWLFunction tau;
	for (int i = 0; i < (int)B.size()-1; ++i)
		tau.AddPiece(LinearFunction(Point2D(B[i], T[i]), Point2D(B[i+1], T[i+1])));
	
	return tau;
}

// Computes the euclidean distance between two points.
double euclidean_distance(double x1, double y1, double x2, double y2)
{
	return sqrt(pow(x1-x2, 2) + pow(y1-y2, 2));
}

inline double get_raw_travel_time_from_td_cost_matrix(
	const json& instance,
	int nb_vertices, 
    int i, 
    int j, 
    size_t time_step
) {
	return instance["td_cost_matrix"][i*nb_vertices + j][time_step];
}

void check_tau(
	const PWLFunction& tau, const Arc& e
) {
	if (tau.check_invariant())
		clog << "*";
	else
	{
		std::ostringstream oss;
		oss << "Invariant error for tau[" << e.tail << "][" << e.head << "]: " << to_string(tau);
		throw runtime_error(oss.str());
	}
}

PWLFunction compute_piecewise_constant_travel_time_function(
	const json& instance, Arc e
) {
	vector<double> B;
	vector<double> T;
	vector<Interval> time_steps = instance["time_steps"];
	int nb_vertices = instance["nb_vertices"];

	for (size_t ts = 0; ts < time_steps.size(); ++ts) 
	{
		// start of time step
		const double t_start = time_steps[ts].left;
		const double travel_time = get_raw_travel_time_from_td_cost_matrix(instance, nb_vertices, e.tail, e.head, ts);
		
		// end of time step
		double t_end = time_steps[ts].right;

		// check if the raw travel time is decreasing
		// in the next time step, if there is one
		if (ts + 1 < time_steps.size()) 
		{
			const double next_travel_time = get_raw_travel_time_from_td_cost_matrix(instance, nb_vertices, e.tail, e.head, ts + 1);
			const double next_t_start = time_steps[ts + 1].left;
			assert(next_t_start == t_end && "Time steps must be contiguous");
			
			if (next_travel_time < travel_time) 
			{
				// travel time decrease must be limited by a
				// slope of -1. Do the math
				t_end = next_travel_time + t_end - travel_time;
			}
			else if (next_travel_time > travel_time)
			{
				// decrease the time step duration by EPS
            	// to avoid discontinuity
				t_end -= EPS;
			}
		}

		B.push_back(t_start);
		T.push_back(travel_time);
		B.push_back(t_end);
		T.push_back(travel_time);
	}

	// filter out duplicates from B and remove associated T values
	vector<double> B_filtered;
	vector<double> T_filtered;
	for (size_t i = 0; i < B.size(); ++i) 
	{
		if (find(B_filtered.begin(), B_filtered.end(), B[i]) == B_filtered.end()) 
		{
			B_filtered.push_back(B[i]);
			T_filtered.push_back(T[i]);
		}
	}

	// Create travel time function.
	PWLFunction tau;
	for (int i = 0; i < (int)B_filtered.size()-1; ++i)
		tau.AddPiece(LinearFunction(Point2D(B_filtered[i], T_filtered[i]), Point2D(B_filtered[i+1], T_filtered[i+1])));	
	
	return tau;
}
} // anonymous namespace

void preprocess_constant_travel_times(nlohmann::json& instance)
{
	clog << " - Constant Travel Times" << endl;
	Interval horizon = instance["horizon"];

	Digraph D = instance;
	Matrix<PWLFunction> tau(D.NbVertices(), D.NbVertices());
	for (Arc e: D.Arcs())
	{
		// We consider the distance as the travel time.
		double distance = euclidean_distance(
			instance["coordinates"][e.head][0], instance["coordinates"][e.head][1],
			instance["coordinates"][e.tail][0], instance["coordinates"][e.tail][1]
		); 
		tau[e.tail][e.head] = PWLFunction::ConstantFunction(distance, Interval(horizon.left, horizon.right - distance));
		check_tau(tau[e.tail][e.head], e);
		clog << "   - Arc " << e.tail << " -> " << e.head << " = " << to_string(tau[e.tail][e.head]) << endl;
	}
	instance["travel_times"] = tau;
}

void preprocess_igp_travel_times(json& instance)
{
	clog << " - IGP Travel Times" << endl;

	Digraph D = instance;
	Matrix<PWLFunction> tau(D.NbVertices(), D.NbVertices());
	for (Arc e: D.Arcs()) 
	{
		tau[e.tail][e.head] = compute_igp_travel_time_function(instance, e);
		check_tau(tau[e.tail][e.head], e);
		clog << "   - Arc " << e.tail << " -> " << e.head << " = " << to_string(tau[e.tail][e.head]) << endl;
	}
	instance["travel_times"] = tau;
}

void preprocess_piecewise_constant_travel_times(json& instance)
{
	clog << " - Piecewise Constant Travel Times" << endl;

	Digraph D = instance;
	Matrix<PWLFunction> tau(D.NbVertices(), D.NbVertices());
	for (Arc e: D.Arcs()) 
	{
		tau[e.tail][e.head] = compute_piecewise_constant_travel_time_function(instance, e);
		check_tau(tau[e.tail][e.head], e);
		clog << "   - Arc " << e.tail << " -> " << e.head << " = " << to_string(tau[e.tail][e.head]) << endl;
	}
	instance["travel_times"] = tau;
}

} // namespace
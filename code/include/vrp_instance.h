//
// Created by Gonzalo Lera Romero.
// Grupo de Optimizacion Combinatoria (GOC).
// Departamento de Computacion - Universidad de Buenos Aires.
//

#ifndef NETWORKS2019_VRP_INSTANCE_H
#define NETWORKS2019_VRP_INSTANCE_H

#include <vector>
#include <goc/goc.h>

// MAX_N is the maximum number of vertices an instance may have, we need this at compilation time for bitset purposes.
#ifndef MAX_N
#define MAX_N 102
#endif

namespace solver
{
typedef double TimeUnit; // Represents time.
typedef double CapacityUnit; // Represents the capacity.
typedef double ProfitUnit; // Represents the profit of vertices.
typedef std::bitset<MAX_N> VertexSet; // Set of vertices.

// This class represents an instance of a vehicle routing problem.
// Considerations:
// 	- It considers two depots (origin and destination).
class VRPInstance : public goc::Printable
{
public:
	goc::Digraph D; // digraph representing the network.
	goc::Vertex o, d; // origin and destination depot.
	TimeUnit T; // end of planning horizon ([0,T]).
	std::vector<goc::Interval> tw; // time window of customers (tw[i] = time window of customer i).
	CapacityUnit Q; // vehicle capacity.
	std::vector<CapacityUnit> q; // demand of customers (q[i] = demand of customer i).
	goc::Matrix<goc::PWLFunction> tau; // tau[i][j](t) = travel time of arc (i, j) if departing from i at t.
	goc::Matrix<goc::PWLFunction> pretau; // pretau[i][j](t) = travel time of arc (i, j) if arriving at j at t.
	goc::Matrix<goc::PWLFunction> dep; // dep[i][j](t) = departing time of arc (i, j) if arriving to j at t.
	goc::Matrix<goc::PWLFunction> arr; // arr[i][j](t) = arrival time of arc (i, j) if departing from i at t.
	goc::Matrix<TimeUnit> LDT; // LDT[i][j] = latest time i can depart from i to reach j before its deadline.
	
	// Returns: the travel time for arc e if departing at t0.
	// If departure at t0 is infeasible, returns INFTY.
	TimeUnit TravelTime(goc::Arc e, TimeUnit t0) const;
	
	// Returns: the travel time for arc e if arriving at tf.
	// If arrival at tf is infeasible, returns INFTY.
	TimeUnit PreTravelTime(goc::Arc e, TimeUnit tf) const;
	
	// Returns: the arrival time for arc e if departing at t0.
	// If departure at t0 is infeasible, returns INFTY.
	TimeUnit ArrivalTime(goc::Arc e, TimeUnit t0) const;
	
	// Returns: the departure time for arc e if arriving at tf.
	// If arrival at tf is infeasible, returns INFTY.
	TimeUnit DepartureTime(goc::Arc e, TimeUnit tf) const;
	
	// Returns: the time we finish visiting the last vertex if departing at t0.
	// If infeasible, returns a route with empty path and INFTY duration.
	TimeUnit ReadyTime(const goc::GraphPath& p, TimeUnit t0=0) const;

	// Returns: the route with minimum duration using path p.
	// If the route is infeasible it returns INFTY.
	goc::Route BestDurationRoute(const goc::GraphPath& p) const;
	
	// Returns: a set of all vertices which are unreachable if departing from v at t0.
	VertexSet Unreachable(goc::Vertex v, TimeUnit t0) const;
	
	// Returns: a set of some vertices which are unreachable if departing from v at t0.
	VertexSet WeakUnreachable(goc::Vertex v, TimeUnit t0) const;
	
	// Prints the JSON representation of the instance.
	virtual void Print(std::ostream& os) const;
};

// Serializes the instance.
void to_json(nlohmann::json& j, const VRPInstance& instance);

// Parses an instance.
void from_json(const nlohmann::json& j, VRPInstance& instance);
} // namespace

#endif //NETWORKS2019_VRP_INSTANCE_H

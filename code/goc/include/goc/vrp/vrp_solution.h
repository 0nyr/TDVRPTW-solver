//
// Created by Gonzalo Lera Romero.
// Grupo de Optimizacion Combinatoria (GOC).
// Departamento de Computacion - Universidad de Buenos Aires.
//

#ifndef GOC_VRP_VRP_SOLUTION_H
#define GOC_VRP_VRP_SOLUTION_H

#include <vector>
#include <iostream>

#include "goc/lib/json.hpp"
#include "goc/print/printable.h"
#include "goc/vrp/route.h"

namespace goc
{
/**
 * @brief Abstract class representing a solution to 
 * an optimization problem.
 * Every solution has a value as a double.
 */
class AbstractSolution: public Printable
{
public:
	// The value of the solution.
	double value;
	
	// Constructor.
	AbstractSolution(double value) : value(value) {}
	
	// Destructor.
	virtual ~AbstractSolution() = default;
	
	// Prints the JSON representation of the solution.
	virtual void Print(std::ostream& os) const = 0;
};

// // Serializes the solution.
// void to_json(nlohmann::json& j, const AbstractSolution& solution)
// {
// 	j["kd_type"] = "abstract_solution";
// 	j["value"] = solution.value;
// }
// // Parses an solution.
// void from_json(const nlohmann::json& j, AbstractSolution& solution)
// {
// 	solution.value = j["value"];
// }
// // Returns: if two solutions are equal.
// bool operator==(const AbstractSolution& s1, const AbstractSolution& s2)
// {
// 	return s1.value == s2.value;
// }

// Represents a solution to a Vehicle Routing Problem.
// This solution has a value, and a set of routes.
// - It knows how to serialize itself in JSON to be compatible with Kaleidoscope kd_type "vrp_solution".
class VRPSolution : public AbstractSolution
{
public:
	std::vector<Route> routes; // Solution routes.
	
	VRPSolution() = default;
	
	// Creates the solution with the specified parameters.
	VRPSolution(double value, const std::vector<Route>& routes);
	
	// Prints the JSON representation of the solution.
	virtual void Print(std::ostream& os) const;
};

// Serializes the solution.
void to_json(nlohmann::json& j, const VRPSolution& solution);

// Parses an solution.
void from_json(const nlohmann::json& j, VRPSolution& solution);

// Returns: if two solutions are equal.
bool operator==(const VRPSolution& s1, const VRPSolution& s2);

} // namespace goc

#endif //GOC_VRP_VRP_SOLUTION_H

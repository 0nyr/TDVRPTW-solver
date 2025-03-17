//
// Created by Gonzalo Lera Romero.
// Grupo de Optimizacion Combinatoria (GOC).
// Departamento de Computacion - Universidad de Buenos Aires.
//

#ifndef NETWORKS2019_PREPROCESS_TRAVEL_TIMES_H
#define NETWORKS2019_PREPROCESS_TRAVEL_TIMES_H

#include <goc/goc.h>

namespace solver
{
// Takes a JSON instance of the vehicle routing problems with the following attributes:
//	- digraph
//	- distances
//	- clusters
//	- cluster_speeds
//	- speed_zones
//	- service_times (optional)
//	- time_windows (optional).
// Adds the travel_times attribute to a JSON instance with a matrix of piecewise linear functions.
void preprocess_travel_times(nlohmann::json& instance);

void preprocess_constant_travel_times(nlohmann::json& instance);
} // namespace

#endif //NETWORKS2019_PREPROCESS_TRAVEL_TIMES_H

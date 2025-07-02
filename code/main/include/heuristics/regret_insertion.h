#pragma once

#include <goc/goc.h>
#include <nyr/nyr.h>

namespace solver {

/**
 * Regret Insertion Heuristic
 * 
 * A variant of the Regret Insertion of Foisy et al. (1993). 
 * Similarly to Pan et al. (2021), we also consider for any given 
 * route the possibility to insert the considered customer at the 
 * end of the route. That way, no seed client vertices are needed, 
 * and no minimal number of routes need to be provided meaning 
 * that this heuristic variant can be used directly to build a 
 * feasible solution from scratch.
 * 
 * Regret is defined as the difference between the
 * minimum duration of the route where inserting the considered
 * customer would be best, and the duration of any other route
 * where inserting the considered customer would be worse, sorted 
 * by Duration. The total regret of a client is the sum of the 
 * regrets for all theses routes.
 * 
 * The heuristic works as follows:
 * 1. For each unvisited client, for each route, compute the minimal 
 *    duration of this route when inserting the client at the best 
 *    position.
 * 2. Compute for this client, and all the routes the sum of the regrets.
 * 3. Select the client with the maximum regret, and insert it at the
 *    best position in the route with the minimum duration.
 * 4. Repeat until all clients are visited.
 * 
 * Note that an empty route is always checked for insertion.
 */
nyr::VRPSolutionDuration regret_insertion_duration(
    const nyr::VRPInstance& vrp,
    const nyr::ARTFs& deltas
);



} // namespace solver

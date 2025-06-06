#pragma once

/**
 * Nyr is a C++ library for solving vehicle routing problems (VRP),
 * in a Time Dependent (TD) context.
 * 
 * It builds on GOC library for graph and combinatorial optimization.
 */

#include "nyr/solutions/route.h"
#include "nyr/solutions/vrp_solution.h"
#include "nyr/solutions/objectives.h"
#include "nyr/solutions/conversions.h"

#include "nyr/params/params.h"

#include "nyr/time/time.h"

#include "nyr/math/fast_math.h"
#include "nyr/math/fast_rng.h"
#include "nyr/math/pwlf.h"

#include "nyr/log/timed_solutions.h"


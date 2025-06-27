#pragma once

#include <cstddef>
#include <vector>
#include <goc/goc.h>

#include "nyr/vrp/types.h"
#include "nyr/solutions/route.h"
#include "nyr/math/ndcpwlf.h"
#include "nyr/vrp/instance.h"

namespace nyr {

/// Arc Ready Time Functions
using ARTFs = goc::Matrix<nyr::NDCPWLF>;

/// Create a matrix of ARTFs for the given VRP instance.
ARTFs make_artfs(const VRPInstance& instance);



} // namespace nyr
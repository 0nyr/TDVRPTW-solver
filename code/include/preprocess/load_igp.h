#pragma once

#include <goc/goc.h>

namespace solver
{

// Takes a JSON instance of a VRP that follows
// the IGP time dependent format and preprocesses it.
void load_igp(nlohmann::json& instance);

} // namespace
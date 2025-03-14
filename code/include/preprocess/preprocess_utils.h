#pragma once

#include <goc/goc.h>

namespace solver
{
// Removes the arc ij from the instance.
void remove_arc(nlohmann::json& instance, goc::Vertex i, goc::Vertex j);
} // namespace
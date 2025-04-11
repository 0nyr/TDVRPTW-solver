#pragma once

#include <chrono>

namespace nyr
{
using Clock = std::chrono::high_resolution_clock;

// Returns the time elapsed in seconds (as a double) from 'start' to now.
inline double seconds_since(const Clock::time_point& start)
{
    return std::chrono::duration<double>(Clock::now() - start).count();
}

std::string generate_timestamp();



} // namespace

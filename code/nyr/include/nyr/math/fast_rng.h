#pragma once

#include <random>
#include <thread>
#include <boost/functional/hash.hpp>

namespace solver
{
// Global seed for reproducibility
inline constexpr uint32_t GLOBAL_SEED = 1;

// Uses Boost's robust hash_combine to derive a per-thread seed from a global seed
inline uint32_t thread_seed(uint32_t global_seed, std::size_t tid) {
    std::size_t combined = global_seed;
    boost::hash_combine(combined, tid);
    return static_cast<uint32_t>(combined); // truncate to 32-bit
}

// Reproducible thread-local RNG returning a number in [0.0, 1.0]
inline double rand01() {
    static thread_local std::mt19937 engine([] {
        std::size_t tid_hash = std::hash<std::thread::id>{}(std::this_thread::get_id());
        return std::mt19937(thread_seed(GLOBAL_SEED, tid_hash));
    }());

    static thread_local std::uniform_real_distribution<double> dist(0.0, 1.0); // inclusive
    return dist(engine);
}
} // namespace

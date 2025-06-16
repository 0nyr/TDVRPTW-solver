#include <goc/goc.h>
#include <nyr/nyr.h>

#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <cassert>

using namespace std;
using namespace goc;
using namespace nyr;
using namespace nlohmann;

// Helper function to print a CPWLF for inspection.
// NOTE: Requires the get_breakpoints() and get_values() getters.
void print_cpwlf(const std::string& name, const nyr::CPWLF& f) {
    std::cout << "--- CPWLF: " << name << " ---\n";
    if (f.empty()) {
        std::cout << "  (Empty Function)\n";
        return;
    }
    std::cout << "  Domain: " << f.get_domain() << "\n";
    std::cout << "  Image:  " << f.get_image() << "\n";
    std::cout << "  Breakpoints (" << f.get_breakpoints().size() << "):\n";
    for (size_t i = 0; i < f.get_breakpoints().size(); ++i) {
        std::cout << "    (" << f.get_breakpoints()[i] << ", " << f.get_values()[i] << ")\n";
    }
    std::cout << "---------------------\n";
}

// Generates a random CPWLF for benchmarking.
// Breakpoints and values are positive and non-zero.
// To ensure chain compositions are meaningful, domain and image are kept similar.
// NOTE: Requires the public constructor CPWLF(breakpoints, values).
nyr::CPWLF generate_random_cpwlf(size_t num_breakpoints, std::mt19937& rng) {
    std::uniform_real_distribution<double> dist(1.0, 100.0);
    
    std::vector<double> breakpoints;
    breakpoints.reserve(num_breakpoints);
    for (size_t i = 0; i < num_breakpoints; ++i) {
        breakpoints.push_back(dist(rng));
    }
    std::sort(breakpoints.begin(), breakpoints.end());
    breakpoints.erase(std::unique(breakpoints.begin(), breakpoints.end(), goc::epsilon_equal), breakpoints.end());

    // Ensure we have at least 2 breakpoints for a valid piece.
    if (breakpoints.size() < 2) {
        breakpoints = {1.0, 100.0};
    }

    std::vector<double> values;
    values.reserve(breakpoints.size());
    for (size_t i = 0; i < breakpoints.size(); ++i) {
        values.push_back(dist(rng));
    }

    return nyr::CPWLF(breakpoints, values);
}

// Runs a single benchmark scenario for both nyr::CPWLF and goc::PWLFunction.
void run_benchmark(size_t num_breakpoints, size_t chain_length, std::mt19937& rng) {
    // Generate the functions for the composition chain
    std::vector<nyr::CPWLF> nyr_functions;
    nyr_functions.reserve(chain_length);
    for (size_t i = 0; i < chain_length; ++i) {
        nyr_functions.push_back(generate_random_cpwlf(num_breakpoints, rng));
    }
    std::vector<goc::PWLFunction> goc_functions;
    goc_functions.reserve(chain_length);
    for (const auto& f : nyr_functions) {
        goc_functions.push_back(f.to_goc_pwl_function());
    }

    std::cout << "| " << std::setw(12) << num_breakpoints
              << " | " << std::setw(14) << chain_length;
    std::cout.flush();

    // Benchmark nyr::CPWLF
    auto start_nyr = std::chrono::high_resolution_clock::now();
    nyr::CPWLF nyr_result = nyr_functions[0];
    for (size_t i = 1; i < chain_length; ++i) {
        nyr_result = nyr_functions[i].compose(nyr_result);
    }
    auto end_nyr = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration_nyr = end_nyr - start_nyr;

    std::cout << " | " << std::setw(9) << std::fixed << std::setprecision(4) << duration_nyr.count() << " s"
              << " | " << std::setw(8) << (nyr_result.empty() ? 0 : nyr_result.nb_pieces());
    std::cout.flush();

    // Benchmark goc::PWLFunction
    auto start_goc = std::chrono::high_resolution_clock::now();
    goc::PWLFunction goc_result = goc_functions[0];
    for (size_t i = 1; i < chain_length; ++i) {
        goc_result = goc_functions[i].Compose(goc_result);
    }
    auto end_goc = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration_goc = end_goc - start_goc;

    std::cout << " | " << std::setw(9) << std::fixed << std::setprecision(4) << duration_goc.count() << " s"
              << " | " << std::setw(8) << goc_result.Pieces().size()
              << " |\n";
    std::cout.flush();
}


void manual_tests() {
    std::cout << "==========================\n";
    std::cout << "=== RUNNING MANUAL TESTS ===\n";
    std::cout << "==========================\n\n";

    // Test 1: Identity and Constant functions
    nyr::CPWLF id = nyr::CPWLF::make_identity({0, 10});
    nyr::CPWLF c5 = nyr::CPWLF::make_constant(5, {0, 10});
    print_cpwlf("Identity f(x)=x", id);
    print_cpwlf("Constant f(x)=5", c5);
    assert(goc::epsilon_equal(id.evaluate(3.5), 3.5));
    assert(goc::epsilon_equal(c5.evaluate(3.5), 5.0));
    std::cout << "Test 1 PASSED: Identity and Constant functions are correct.\n\n";

    // Test 2: Simple composition f(g(x)) where f=|x| and g=x-5
    nyr::CPWLF f_abs({-10, 0, 10}, {10, 0, 10}); // f(x)=|x| on [-10, 10]
    nyr::CPWLF g_shift = nyr::CPWLF::make_identity({-5, 15});
    g_shift = nyr::CPWLF({-5, 15}, {-10, 10}); // g(x) = x-5 on [-5, 15]
    
    nyr::CPWLF fog = f_abs.compose(g_shift);
    print_cpwlf("f(x)=|x|", f_abs);
    print_cpwlf("g(x)=x-5", g_shift);
    print_cpwlf("f(g(x))=|x-5|", fog);
    
    // Check points: f(g(10)) = |10-5| = 5. f(g(0)) = |0-5| = 5. f(g(5))=|5-5|=0
    assert(goc::epsilon_equal(fog.evaluate(10), 5.0));
    assert(goc::epsilon_equal(fog.evaluate(0), 5.0));
    assert(goc::epsilon_equal(fog.evaluate(5), 0.0));
    std::cout << "Test 2 PASSED: Composition |x-5| is correct.\n\n";

    // Test 3: Composition with a constant function
    nyr::CPWLF fog_const = f_abs.compose(c5); // f(5) = |5| = 5
    print_cpwlf("f(g(x)) where g(x)=5", fog_const);
    assert(goc::epsilon_equal(fog_const.evaluate(5), 5.0));
    assert(fog_const.compute_domain().IsPoint());
    std::cout << "Test 3 PASSED: Composition with a constant is correct.\n\n";

    // Test 4: Composition where g's image doesn't intersect f's domain
    nyr::CPWLF f_far_domain({100, 110}, {100, 110});
    nyr::CPWLF fog_empty = f_far_domain.compose(g_shift);
    print_cpwlf("f with domain [100,110]", f_far_domain);
    print_cpwlf("g with image [-10,10]", g_shift);
    print_cpwlf("f(g(x)) -> empty", fog_empty);
    assert(fog_empty.empty());
    std::cout << "Test 4 PASSED: Composition with non-intersecting domains is empty.\n\n";
    
    // Test 5: Normalization test
    nyr::CPWLF non_normalized({0, 1, 2, 3}, {0, 1, 2, 3}); // Redundant points
    nyr::CPWLF normalized = non_normalized.compose(nyr::CPWLF::make_identity({0, 3}));
    print_cpwlf("Normalized f(x)=x", normalized);
    assert(normalized.nb_pieces() == 1);
    assert(normalized.get_breakpoints().size() == 2);
    std::cout << "Test 5 PASSED: Normalization removes redundant points.\n\n";

    std::cout << "==========================\n";
    std::cout << "=== ALL TESTS PASSED! ====\n";
    std::cout << "==========================\n\n";
}

int main() {
    // Run manual tests first to ensure correctness
    manual_tests();

    // Setup for benchmarking
    std::cout << "================================\n";
    std::cout << "=== RUNNING BENCHMARKING ===\n";
    std::cout << "================================\n\n";

    std::random_device rd;
    std::mt19937 rng(rd());

    std::vector<size_t> breakpoint_counts = {5, 10, 20, 50, 100, 500, 1000};
    std::vector<size_t> chain_lengths = {5, 10, 20, 30, 50, 100};

    std::cout << "+--------------+----------------+-----------+----------+-----------+----------+\n";
    std::cout << "| #Breakpoints | #Compositions  |  NYR Time | NYR Size |  GOC Time | GOC Size |\n";
    std::cout << "+--------------+----------------+-----------+----------+-----------+----------+\n";

    for (size_t bkpt_count : breakpoint_counts) {
        for (size_t chain_len : chain_lengths) {
            run_benchmark(bkpt_count, chain_len, rng);
        }
    }
    
    std::cout << "+--------------+----------------+--------------------+--------------------+\n";
    
    return 0;
}

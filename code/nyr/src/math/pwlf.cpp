#include "nyr/math/pwlf.h"

#include <stdexcept>
#include <limits>

using namespace std;
using namespace goc;
using namespace nlohmann;

namespace nyr {

// Static factory for constant function f(x) = a
CPWLF CPWLF::make_constant(double a, goc::Interval domain) {
    if (domain.Empty()) {
        return CPWLF(); // Return empty function
    }
    
    CPWLF result;
    result.breakpoints_ = {domain.left, domain.right};
    result.values_ = {a, a};
    return result;
}

// Static factory for identity function f(x) = x
CPWLF CPWLF::make_identity(goc::Interval domain) {
    if (domain.Empty()) {
        return CPWLF(); // Return empty function
    }
    
    CPWLF result;
    result.breakpoints_ = {domain.left, domain.right};
    result.values_ = {domain.left, domain.right};
    return result;
}

// Check if function is empty
bool CPWLF::empty() const {
    return breakpoints_.empty();
}

// Number of pieces = breakpoints - 1
size_t CPWLF::nb_pieces() const {
    return empty() ? 0 : breakpoints_.size() - 1;
}

// Domain: [first_breakpoint, last_breakpoint] since breakpoints are sorted
goc::Interval CPWLF::domain() const {
    if (empty()) {
        return Interval(); // Return empty interval [INFTY, -INFTY]
    }
    return goc::Interval(breakpoints_.front(), breakpoints_.back());
}

// Image: [min_value, max_value]
goc::Interval CPWLF::image() const {
    if (empty()) {
        return Interval(); // Return empty interval [INFTY, -INFTY]
    }
    auto minmax = std::minmax_element(values_.begin(), values_.end());
    return goc::Interval(*minmax.first, *minmax.second);
}

// O(log n) evaluation using binary search and linear interpolation
// Throws std::out_of_range if x is outside the domain.
// Precondition: x \in dom(f)
double CPWLF::evaluate(double x) const {
    if (empty()) {
        throw std::out_of_range("Cannot evaluate empty CPWLF");
    }
    
    // Check domain bounds using epsilon comparisons
    // x < breakpoints_.front() || x > breakpoints_.back()
    if (epsilon_smaller(x, breakpoints_.front()) || 
        epsilon_bigger(x, breakpoints_.back())
    ) {
        throw std::out_of_range("x is outside CPWLF domain");
    }
    
    // Binary search to find the right segment
    // Find first breakpoint that is epsilon_bigger than x
    // NOTE: Do not use epsilon comparison here (better optimization)
    // and perform check after finding the segment.
    auto it = std::upper_bound(breakpoints_.begin(), breakpoints_.end(), x);
    
    // Handle edge case where upper_bound returns end()
    // NOTE: critical safety check to prevent undefined behavior 
    // when dereferencing the iterator.
    if (it == breakpoints_.end()) {
        return values_.back();
    }
    
    // Handle case where x is very close to a breakpoint
    if (epsilon_equal(x, *it)) {
        size_t idx = std::distance(breakpoints_.begin(), it);
        return values_[idx];
    }
    
    // Get indices for the segment [x_i, x_{i+1}]
    size_t right_idx = std::distance(breakpoints_.begin(), it);
    size_t left_idx = right_idx - 1;
    
    // Linear interpolation between (x_i, y_i) and (x_{i+1}, y_{i+1})
    double x_left = breakpoints_[left_idx];
    double x_right = breakpoints_[right_idx];
    double y_left = values_[left_idx];
    double y_right = values_[right_idx];
    
    #ifndef NDEBUG
        // Handle degenerate case using epsilon comparison
        if (epsilon_equal(x_right, x_left)) {
            return y_left;
        }
    #endif
    
    // Linear interpolation: y = y_left + (y_right - y_left) * (x - x_left) / (x_right - x_left)
    double t = (x - x_left) / (x_right - x_left);
    return y_left + t * (y_right - y_left);
}

bool CPWLF::check_invariant() const {
    if (breakpoints_.size() != values_.size()) return false;
    if (breakpoints_.size() < 2 && !empty()) return false;

    // Check no duplicate breakpoints
    for (size_t i = 1; i < breakpoints_.size(); ++i) {
        if (epsilon_equal(breakpoints_[i], breakpoints_[i-1])) {
            return false; // Duplicate breakpoints found
        }
    }
    
    // Check if breakpoints are sorted
    for (size_t i = 1; i < breakpoints_.size(); ++i) {
        if (epsilon_smaller_equal(breakpoints_[i], breakpoints_[i-1])) {
            return false;
        }
    }
    return true;
}

} // namespace nyr
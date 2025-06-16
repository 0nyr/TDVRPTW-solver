#include "nyr/math/cpwlf.h"

#include <stdexcept>
#include <limits>
#include <algorithm>
#include <vector>

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
    result.domain_ = Interval(domain.left, domain.right);
    result.image_ = Interval(a, a);
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
    result.domain_ = Interval(domain.left, domain.right);
    result.image_ = Interval(domain.left, domain.right);
    return result;
}

// Constructor for 2D continuous piecewise linear function (CPWLF)
CPWLF::CPWLF(
    const vector<double>& breakpoints,
    const vector<double>& values
) {
    if (breakpoints.empty() || values.empty() || breakpoints.size() != values.size()) {
        throw invalid_argument("Breakpoints and values must be non-empty and of the same size.");
    }
    
    #ifndef NDEBUG
    // Check if breakpoints are sorted and unique
    for (size_t i = 1; i < breakpoints.size(); ++i) {
        if (epsilon_smaller_equal(breakpoints[i], breakpoints[i-1])) {
            throw invalid_argument("Breakpoints must be sorted and unique.");
        }
    }
    #endif
    
    breakpoints_ = breakpoints;
    values_ = values;
    
    // Compute domain and image
    domain_ = compute_domain();
    image_ = compute_image();
    
    #ifndef NDEBUG
    // Check invariant
    if (!check_invariant()) {
        throw runtime_error("Invariant check failed after construction.");
    }
    #endif
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
goc::Interval CPWLF::compute_domain() const {
    if (empty()) {
        return Interval(); // Return empty interval [INFTY, -INFTY]
    }
    return goc::Interval(breakpoints_.front(), breakpoints_.back());
}

// Image: [min_value, max_value]
goc::Interval CPWLF::compute_image() const {
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

    // Check if stored domain is consistent with breakpoints
    if (empty()) {
        if (!domain_.Empty()) {
            return false; // Empty function should have empty domain
        }
    } else {
        if (empty() && !domain_.Empty()) {
             return false; // Non-empty domain for empty function
        } 
    }

    // Check if stored image is consistent with values
    if (empty()) {
        if (!image_.Empty()) {
            return false; // Empty function should have empty image
        }
    } else {
        auto minmax = std::minmax_element(values_.begin(), values_.end());
        if (image_.left != *minmax.first || image_.right != *minmax.second) {
            return false; // Image does not match values
        }
    }

    return true;
}

CPWLF CPWLF::compose(const CPWLF& g) const {
    const auto& f = *this;

    // 1. Pre-check
    // If either function is empty, or if the image of g does not intersect the domain of f,
    // the composition is an empty function.
    if (f.empty() || g.empty() || !f.domain_.Intersects(g.image_)) {
        return CPWLF();
    }

    std::vector<double> potential_bkpts;

    // 2. Collect Breakpoints
    // Source A: Add all breakpoints from g.
    for (double bkpt : g.breakpoints_) {
        potential_bkpts.push_back(bkpt);
    }
    
    // Source B: Find where g(x) hits a breakpoint of f.
    for (double f_y_bkpt : f.breakpoints_) {
        // We need to solve g(x) = f_y_bkpt for each piece of g.
        for (size_t i = 0; i < g.nb_pieces(); ++i) {
            double g_x1 = g.breakpoints_[i];
            double g_x2 = g.breakpoints_[i+1];
            double g_y1 = g.values_[i];
            double g_y2 = g.values_[i+1];

            // Check if the target y-value is within the vertical range of this piece of g, using epsilon comparisons.
            double min_gy = std::min(g_y1, g_y2);
            double max_gy = std::max(g_y1, g_y2);
            if (epsilon_bigger_equal(f_y_bkpt, min_gy) && epsilon_smaller_equal(f_y_bkpt, max_gy)) {
                // If the piece is horizontal
                if (epsilon_equal(g_y1, g_y2)) {
                    // If g(x) is constant and this constant is a breakpoint of f,
                    // the endpoints of this segment are already included from g.breakpoints_.
                    continue;
                }

                // Solve for x using inverse linear interpolation.
                // f_y_bkpt = g_y1 + (g_y2 - g_y1) * (x - g_x1) / (g_x2 - g_x1)
                double x_sol = g_x1 + (f_y_bkpt - g_y1) * (g_x2 - g_x1) / (g_y2 - g_y1);
                
                // Add the solution if it's within the piece's domain interval [g_x1, g_x2], using epsilon comparisons.
                if (epsilon_bigger_equal(x_sol, g_x1) && epsilon_smaller_equal(x_sol, g_x2)) {
                    potential_bkpts.push_back(x_sol);
                }
            }
        }
    }
    
    // 3. Sort and Unify
    std::sort(potential_bkpts.begin(), potential_bkpts.end());
    potential_bkpts.erase(
        std::unique(potential_bkpts.begin(), potential_bkpts.end(), 
                    [](double a, double b) { return epsilon_equal(a, b); }),
        potential_bkpts.end()
    );

    // 4. Filter, Evaluate, and Build Raw Function
    CPWLF raw_fog;
    for (double x_bkpt : potential_bkpts) {
        // A breakpoint is valid only if g(x) is in the domain of f.
        // g.evaluate() and f.domain_.Includes() already handle boundary cases correctly.
        double g_val = g.evaluate(x_bkpt);
        if (f.domain_.Includes(g_val)) {
            raw_fog.breakpoints_.push_back(x_bkpt);
            raw_fog.values_.push_back(f.evaluate(g_val));
        }
    }

    if (raw_fog.empty() || raw_fog.nb_pieces() == 0) {
        return CPWLF();
    }

    // 5. Normalize
    // Remove intermediate breakpoints if the slope doesn't change.
    CPWLF fog;
    if (raw_fog.nb_pieces() < 2) {
        fog = raw_fog;
    } else {
        fog.breakpoints_.push_back(raw_fog.breakpoints_.front());
        fog.values_.push_back(raw_fog.values_.front());

        for (size_t i = 1; i < raw_fog.nb_pieces(); ++i) {
            double x_prev = raw_fog.breakpoints_[i-1];
            double y_prev = raw_fog.values_[i-1];
            double x_curr = raw_fog.breakpoints_[i];
            double y_curr = raw_fog.values_[i];
            double x_next = raw_fog.breakpoints_[i+1];
            double y_next = raw_fog.values_[i+1];

            // Denominators cannot be zero because of the unique-sort step.
            double slope1 = (y_curr - y_prev) / (x_curr - x_prev);
            double slope2 = (y_next - y_curr) / (x_next - x_curr);

            // If the slopes are different, the current breakpoint is necessary.
            if (!epsilon_equal(slope1, slope2)) {
                fog.breakpoints_.push_back(x_curr);
                fog.values_.push_back(y_curr);
            }
        }
        
        fog.breakpoints_.push_back(raw_fog.breakpoints_.back());
        fog.values_.push_back(raw_fog.values_.back());
    }

    // 6. Construct: Update cached domain and image
    if (!fog.empty()) {
        fog.domain_ = fog.compute_domain();
        fog.image_ = fog.compute_image();
    }

    return fog;
}

void CPWLF::Print(std::ostream& os) const {
    clog << "CPWLF Image: " << image_ << ", Domain: " << domain_ << "]" << endl;
    print_padded_vectors(clog, breakpoints_, values_);
}

bool CPWLF::operator==(const CPWLF& other) const {
    // Check if breakpoints and values are the same
    return (
        (breakpoints_ == other.breakpoints_) && 
        (values_ == other.values_)
    );
}

goc::PWLFunction CPWLF::to_goc_pwl_function() const {
    goc::PWLFunction pwlf;
    if (empty()) {
        return pwlf; // Return empty PWLFunction
    }
    
    // Convert breakpoints and values to goc::PWLFunction
    for (size_t i = 0; i < breakpoints_.size() - 1; ++i) {
        pwlf.AddPiece(goc::LinearFunction(
            goc::Point2D(breakpoints_[i], values_[i]),
            goc::Point2D(breakpoints_[i + 1], values_[i + 1])
        ));
    }
    
    return pwlf;
}

} // namespace nyr
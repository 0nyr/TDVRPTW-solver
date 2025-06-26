#pragma once

#include <vector>
#include <goc/goc.h>

namespace nyr
{

/**
 * ### Continuous Piecewise Linear Function (CPWLF).
 * 
 * This class represents a continuous piecewise linear function.
 * It is defined by a set of breakpoints and corresponding values.
 * The function is continuous and linear between each pair of 
 * consecutive breakpoints, and its slope is always bounded 
 * (non-infinite).
 * 
 * Note that the function is stored normalized. A function is 
 * normalized iif no two consecutive pieces have the same
 * slope, intercept, and share the end and beginning of their domains.
 * 
 * Contains `p` pieces for `b = p + 1` breakpoints.
 */
class CPWLF : public goc::Printable {

private:
    std::vector<double> breakpoints_;  // Sorted x-values
    std::vector<double> values_;       // Corresponding y-values
    goc::Interval domain_, image_;
    
public:
    // Returns: f(x)=a with the specific domain.
	static CPWLF make_constant(double a, goc::Interval domain);

    // Returns: f(x)=x with the specific domain.
	static CPWLF make_identity(goc::Interval domain);

    // Constructor for 2D continuous piecewise linear function
    // Precondition: The list of breakpoints must be sorted and unique.
    CPWLF(
        const std::vector<double>& breakpoints,
        const std::vector<double>& values
    );

    // Return empty
    CPWLF() = default;

    // Returns true if the function is empty (no pieces).
    bool empty() const;

    // Returns the number of pieces in the function.
    size_t nb_pieces() const;

	// Returns: the smallest interval [m, M] that includes all pieces domains.
	// Observation: if Empty() then returns [INFTY, -INFTY].
	goc::Interval compute_domain() const;
	
	// Returns: the smallest interval [m, M] that includes all pieces images.
	// Observation: if Empty() then returns [INFTY, -INFTY].
	goc::Interval compute_image() const;

    // O(log n) evaluation using binary search and linear interpolation
    // Throws std::out_of_range if x is outside the domain.
    // Precondition: x \in dom(f)
    double evaluate(double x) const;

    // Checks if the function is well-formed.
    bool check_invariant() const;

	// Returns: the composition of this function (f) and g, i.e. fog(x) == f(g(x)).
	// Observation: the domain of the new function are those x such that g(x) \in dom(f).
    CPWLF compose(const CPWLF& g) const;

    goc::PWLFunction to_goc_pwl_function() const;

    void Print(std::ostream& os) const;

    // Getters
    const goc::Interval& get_domain() const { return domain_; }
    const goc::Interval& get_image() const { return image_; }
    const std::vector<double>& get_breakpoints() const { return breakpoints_; }
    const std::vector<double>& get_values() const { return values_; }

    // Returns: if the function equals another function.
    bool operator==(const CPWLF& other) const;
};


} // namespace nyr
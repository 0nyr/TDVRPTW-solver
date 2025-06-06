#pragma once

#include <vector>
#include <goc/goc.h>

namespace nyr
{

/**
 * Continuous Piecewise Linear Function (CPWLF).
 * 
 * Contains `p` pieces for `b=p+1` breakpoints.
 */
class CPWLF {
private:
    std::vector<double> breakpoints_;  // Sorted x-values
    std::vector<double> values_;       // Corresponding y-values
    
public:
    // Returns: f(x)=a with the specific domain.
	static CPWLF make_constant(double a, goc::Interval domain);

    // Returns: f(x)=x with the specific domain.
	static CPWLF make_identity(goc::Interval domain);

    // Return empty
    CPWLF() = default;

    // Returns true if the function is empty (no pieces).
    bool empty() const;

    // Returns the number of pieces in the function.
    size_t nb_pieces() const;

	// Returns: the smallest interval [m, M] that includes all pieces domains.
	// Observation: if Empty() then returns [INFTY, -INFTY].
	goc::Interval domain() const;
	
	// Returns: the smallest interval [m, M] that includes all pieces images.
	// Observation: if Empty() then returns [INFTY, -INFTY].
	goc::Interval image() const;

    // O(log n) evaluation using binary search and linear interpolation
    // Throws std::out_of_range if x is outside the domain.
    // Precondition: x \in dom(f)
    double evaluate(double x) const;

    // Checks if the function is well-formed.
    bool CPWLF::check_invariant() const;
};


}
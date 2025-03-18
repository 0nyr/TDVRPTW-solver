#pragma once

#include <vector>
#include <iostream>

#include "goc/lib/json.hpp"
#include "goc/math/interval.h"
#include "goc/print/printable.h"

namespace goc 
{

// This class represent a closed partitioned interval [left, ..., right].
// The interval is subdivided by a sorted vector of breakpoints that represent
// a sorted union of successive intervals.
// Invariant: left = breakpoints[0] < breakpoints[1] < … < breakpoints.back() = right.
// Note that if the interval is a point, left = right = breakpoints[0].
class PartitionedInterval : public Printable 
{
public:
    // Default constructor: creates an empty partitioned interval.
    PartitionedInterval();

    // Constructs a PartitionedInterval with a sorted vector of breakpoints.
    PartitionedInterval(const std::vector<double>& breakpoints);

    // Adds a breakpoint to the partitioned interval.
    // Keeps the sorted breakpoints invariant.
    void add(double breakpoint);

    // Adds an interval to the partitioned interval.
    // Keeps the sorted breakpoints invariant.
    void add(const Interval& interval);

    bool empty() const;

    // Returns: if the domain is [a]
    bool is_point() const;

    // Returns the overall interval as an Interval.
    Interval bound() const;

    // Returns the interval segment in which 'value' belongs.
    // If the value is not in the domain, the empty interval is returned.
    // Note: if 'value' equals a partition breakpoint, the next interval is returned,
    // or [breakpoint, breakpoint] if it is the last one.
    Interval find_interval(double value) const;

    // Prints the PartitionedInterval.
    // Format: [left, breakpoints[1], ..., right].
    virtual void Print(std::ostream& os) const;

    // Getters for the overall bounds and the breakpoints.
    double left() const;
    double right() const;
    const std::vector<double>& get_breakpoints() const;

private:
    // Stores the breakpoints in strictly increasing order.
    // They subdivide the interval [left, right] into segments:
    // left = breakpoints[0] < breakpoints[1] < … < breakpoints.back() = right.
    std::vector<double> breakpoints_;
};

// JSON format: [left, breakpoints[1], ..., right].
void from_json(const nlohmann::json& j, PartitionedInterval& i);

void to_json(nlohmann::json& j, const PartitionedInterval& i);
} // namespace goc

// Adding to namespace std to not conflict with overload.
namespace std
{
// Returns: pi.left
inline double min(const goc::PartitionedInterval& p) { return p.left(); }

// Returns: pi.right
inline double max(const goc::PartitionedInterval& p) { return p.right(); }
} // namespace std
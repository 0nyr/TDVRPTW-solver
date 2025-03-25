#pragma once

#include <goc/goc.h>

namespace solver
{
// Enum of the strategies to use for the TDNGNeighborhoods.
enum class TDNGNeighborhoodsTimeStrategy
{
    TimeStepSpecific, // Time-dependent neighborhoods are specific for each time step.
    PartitionedHorizon, // Time-dependent neighborhoods are given for a partitioned horizon into periods of same duration.
    RepresentativeTimePeriods, // Time-dependent neighborhoods are given for predefined time periods, with predetermined time periods less numerous and more representative of the time horizon changes in travel times.
    Static // Time-dependent neighborhoods are static, they do not change over time.
};

// Enum of aggregation strategies for the TDNGNeighborhoods calculation of "closest neighbors".
enum class TDNGNeighborhoodsAggregationStrategy
{
    NoAggregation, // Does not aggregate travel times per period. Use directly the time dependent travel times.
    MinTravelTime, // Aggregates travel times per period by the minimum travel time.
    AverageTravelTime, // Aggregates travel times per period by the average travel time.
    MaxTravelTime, // Aggregates travel times per period by the maximum travel time.
};

// Determine the NG neighborhoods of each vertex.
void preprocess_ng_neighborhoods(nlohmann::json& instance);
} // namespace
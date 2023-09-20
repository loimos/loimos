/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef DEFS_H_
#define DEFS_H_

#include <vector>
#include <string>
#include "pup.h"

#include "Types.h"
#include <limits>

// Concat two parts of variable names (for use with other macros)
// in-direction neccessary so that any macros passed in will be expanded prior
// to being concatenated
#define CONCAT(x, y) CONCAT_(x, y)
#define CONCAT_(x, y) x ## y
// #define CK_SUM_COUNTER(type) CONCAT(CkReduction::sum_, COUNTER_REDUCTION_TYPE)

// Debug levels
#define DEBUG_BASIC 1
#define DEBUG_VERBOSE 2
#define DEBUG_LOCATION_SUMMARY 3  // Sumary of interaction counts per location
#define DEBUG_PER_INTERACTION 4  // Saves all interactions to a file
#define DEBUG_PER_CHARE 5
#define DEBUG_PER_OBJECT 6

// Disease states
#define SUSCEPTIBLE 0
#define EXPOSED 1
#define INFECTIOUS 2
#define RECOVERED 3
#define NUMBER_STATES 4

#define INCUBATION_PERIOD 2
#define INFECTION_PERIOD 4

// Event types
#define ARRIVAL 1
#define DEPARTURE 0

// Time
const Time DAY_LENGTH = 3600 * 24;
const Time HOUR_LENGTH = 3600;
const Time MINUTE_LENGTH = 60;
#define DAYS_IN_WEEK 7

// Indices of attribute columns in the appropriate csvs
#define AGE_CSV_INDEX 0

// Data loading
#define EMPTY_VISIT_SCHEDULE std::numeric_limits<CacheOffset>::max()
#define CSV_DELIM ','

#define DAYS_TO_SEED_INFECTION 7
#if ENABLE_DEBUG == DEBUG_PER_INTERACTION
  #define INITIAL_INFECTIONS_PER_DAY 0
#else
  #define INITIAL_INFECTIONS_PER_DAY 10
#endif
#define INITIAL_INFECTIONS (INITIAL_INFECTIONS_PER_DAY * DAYS_TO_SEED_INFECTION)

// Previous, completely algebraic, paritioning scheme:
// Some functions are still in use in the new scheme but most should be
// consider depricated outside that scope
Id getNumElementsPerPartition(Id numElements, PartitionId numPartitions);
PartitionId getPartitionIndex(Id globalIndex, Id numElements,
    PartitionId numPartitions, Id offset);
Id getFirstIndex(PartitionId partitionIndex, Id numElements,
    PartitionId numPartitions, Id offset);
Id getNumLocalElements(Id numElements, PartitionId numPartitions,
    PartitionId partitionIndex);
Id getGlobalIndex(Id localIndex, PartitionId partitionIndex, Id numElements,
    PartitionId numPartitions, Id offset);
Id getLocalIndex(Id globalIndex, PartitionId partitionIndex, Id numElements,
    PartitionId numPartitions, Id offset);

// New, offset-based partitioning scheme:
Id getLocalIndex(Id globalIndex, PartitionId PartitionId,
    const std::vector<Id> &offsets);
Id getGlobalIndex(Id localIndex, PartitionId PartitionId,
    const std::vector<Id> &offsets);
PartitionId getPartition(Id globalIndex, const std::vector<Id> &offsets);
Id getPartitionSize(PartitionId partitionIndex,
    Id numObjects, const std::vector<Id> &offsets);

template <typename T>
bool outOfBounds(T lower, T upper, T value) {
  return lower > value || upper <= value;
}

#endif  // DEFS_H_

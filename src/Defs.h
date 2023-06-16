/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef DEFS_H_
#define DEFS_H_

#include <vector>
#include <string>

// Debug levels
#define DEBUG_BASIC 1
#define DEBUG_VERBOSE 2
#define DEBUG_PER_CHARE 3
#define DEBUG_PER_OBJECT 4

// Disease states
#define SUSCEPTIBLE 0
#define EXPOSED 1
#define INFECTIOUS 2
#define RECOVERED 3
#define NUMBER_STATES 4

#define INCUBATION_PERIOD 2
#define INFECTION_PERIOD 4

// Event types
using EventType = char;
#define ARRIVAL 0
#define DEPARTURE 1

// Time
using Time = int32_t;
const Time DAY_LENGTH = 3600 * 24;
const Time HOUR_LENGTH = 3600;
const Time MINUTE_LENGTH = 60;
#define DAYS_IN_WEEK 7

// Indices of attribute columns in the appropriate csvs
#define AGE_CSV_INDEX 0
#define SIMULTANEOUS_MAX_VISITS_CSV_INDEX 1

// Data loading
#define EMPTY_VISIT_SCHEDULE 0xFFFFFFFF
#define CSV_DELIM ','

#define PERCENTAGE_OF_SEEDING_LOCATIONS 0.001
#define INITIAL_INFECTIOUS_PROBABILITY 0.01
#define DAYS_TO_SEED_INFECTION 7
#define INITIAL_INFECTIONS_PER_DAY 10
#define INITIAL_INFECTIONS (INITIAL_INFECTIONS_PER_DAY * DAYS_TO_SEED_INFECTION)

int getNumElementsPerPartition(int numElements, int numPartitions);

int getNumLocalElements(int numElements, int numPartitions, int partitionIndex);

int getPartitionIndex(int globalIndex, int numElements, int numPartitions, int offset);

int getLocalIndex(int globalIndex, int numElements, int numPartitions, int offset);

int getGlobalIndex(int localIndex, int partitionIndex, int numElements,
  int numPartitions, int offset);

#endif  // DEFS_H_

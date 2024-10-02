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
#define DEBUG_PER_CHARE 4
#define DEBUG_PER_OBJECT 5

// Output types - these are flags that can be or-ed together
// For now, always write out the default output (daily state transition summaries)
#define OUTPUT_TRANSITIONS 1
#define OUTPUT_EXPOSURES 2
#define OUTPUT_OVERLAPS 4

// Tracing levels
#define TRACE_BASIC 1
#define TRACE_MEMORY 2

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
const Time N_VISIT_BINS = 24 * 60 * 2;
const double VISIT_BIN_DURATION = DAY_LENGTH / N_VISIT_BINS;

// Data loading
#define EMPTY_VISIT_SCHEDULE std::numeric_limits<CacheOffset>::max()
#define CSV_DELIM ','
#define FILE_READ_ERROR -1

#endif  // DEFS_H_

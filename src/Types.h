/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef TYPES_H_
#define TYPES_H_

#include "charm++.h"
#include "pup_stl.h"

#include <cstdint>
#include <string>

// For object I/O cache
using CacheOffset = uint64_t;

// For locating people or locations
using Id = int64_t;
#define ID_PRINT_TYPE "%ld"
#define ID_PROTOBUF_TYPE int64
#define ID_PROTOBUF_TYPE_CAP Int64
#define ID_REDUCTION_TYPE long  // NOLINT(runtime/int)
#define ID_PARSE std::stol

using PartitionId = int;
#define PARTITION_ID_PRINT_TYPE "%d"
#define PARTITION_ID_PARSE std::stoi

// Disease states
using DiseaseState = int;

// Event types
using EventType = char;
using Time = int32_t;
#define TIME_PRINT_TYPE "%d"
#define TIME_PROTOBUF_TYPE int32
#define TIME_PROTOBUF_TYPE_CAP Int32
#define TIME_PARSE std::atoi

// For counting events (interactions, visits, exposures...)
using Counter = double;
#define COUNTER_PRINT_TYPE "%0.0f"
#define COUNTER_REDUCTION_TYPE double

template <class T>
struct Grid {
  T width;
  T height;

  T area() {
    return width * height;
  }

  template <class S>
  bool operator>=(const Grid<S> &rhs) {
    return width >= rhs.width && height <= rhs.height;
  }

  template <class S>
  bool isDivisible(const Grid<S> &rhs) {
    return (width % rhs.width == 0)
      && (height % rhs.height == 0);
  }
};

struct OnTheFlyArguments {
  Grid<Id> personGrid;
  Grid<Id> locationGrid;
  Grid<PartitionId> locationPartitionGrid;
  Grid<Id> localLocationGrid;

  Id averageVisitsPerDay;
};

struct Arguments {
  // Arguments needed for all runs
  PartitionId numPersonPartitions;
  PartitionId numLocationPartitions;
  Time numDays;
  Time numDaysWithDistinctVisits;
  Time numDaysToSeedOutbreak;
  Id numInitialInfectionsPerDay;
  int seed;

  bool hasIntervention;
  int contactModelType;

  std::string diseasePath;
  std::string interventionPath;
  std::string outputPath;

  bool isOnTheFlyRun;
  struct OnTheFlyArguments onTheFly;
  std::string scenarioPath;

  Arguments() {}
  explicit Arguments(CkMigrateMessage *msg) {}
};
PUPbytes(Arguments);

struct Profile {
  Counter totalVisits;
  Counter totalInteractions;
  Counter totalExposures;
  Counter totalExposureDuration;
  double simulationStartTime;
  double iterationStartTime;
  double stepStartTime;

  double visitsTime;
  double interactionsTime;
  double eodTime;
};

#endif  // TYPES_H_

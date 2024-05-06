/* Copyright 2020-2024 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef READERS_PARSE_H__
#define READERS_PARSE_H__

#include "../Types.h"
#include "charm++.h"
#include "pup_stl.h"

#include <string>

struct OnTheFlyArguments {
  Grid<Id> personGrid;
  Grid<Id> locationGrid;
  Grid<PartitionId> locationPartitionGrid;
  Grid<Id> localLocationGrid;

  double averageVisitsPerDay;
};
PUPbytes(OnTheFlyArguments);

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

  void pup(PUP::er &p) {  // NOLINT(runtime/references)
    p | numPersonPartitions;
    p | numLocationPartitions;
    p | numDays;
    p | numDaysWithDistinctVisits;
    p | numDaysToSeedOutbreak;
    p | numInitialInfectionsPerDay;
    p | seed;
    p | hasIntervention;
    p | contactModelType;
    p | diseasePath;
    p | interventionPath;
    p | outputPath;
    p | scenarioPath;
    p | isOnTheFlyRun;
    p | onTheFly;
  }
};

void parse(int argc, char **argv, Arguments *args);

#endif  // READERS_PARSE_H__

/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef READERS_PREPROCESS_H_
#define READERS_PREPROCESS_H_

#include "../Types.h"

#include <tuple>
#include <string>

// Main entry point.
std::tuple<Id, Id, std::string> buildCache(std::string scenarioPath, Id numPeople,
  int peopleChares, Id numLocations, int numLocationChares, int numDays);

// Helper functions.
Id buildObjectLookupCache(std::string inputPath, std::string outputPath, Id numObjs,
  int numChares, std::string pathToCsvDefinition);
void buildActivityCache(std::string inputPath, std::string outputPath, Id numPeople,
  int numDays, Id firstPersonIdx, std::string pathToCsvDefinition);
int getDay(Time timeInSeconds);
std::string getScenarioId(Id numPeople, int numPeopleChares, Id numLocations,
  int numLocationChares);

#endif  // READERS_PREPROCESS_H_

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
#include <google/protobuf/text_format.h>

// Main entry point.
std::tuple<Id, Id, std::string> buildCache(std::string scenarioPath,
    Id numPeople, const std::vector<Id> &personPartitionOffsets,
    Id numLocations, const std::vector<Id> &locationPartitionOffsets, int numDays);

// Helper functions.
Id buildObjectLookupCache(Id numObjs, const std::vector<Id> offsets,
  std::string metadataPath, std::string inputPath, std::string outputPath);
void buildActivityCache(Id numPeople, int numDays, Id firstPersonIdx,
  std::string metadataPath, std::string inputPath, std::string outputPath);
int getDay(Time timeInSeconds);
std::string getScenarioId(Id numPeople, PartitionId numPeopleChares, Id numLocations,
  PartitionId numLocationChares);

#endif  // READERS_PREPROCESS_H_

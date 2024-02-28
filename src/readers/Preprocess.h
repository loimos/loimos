/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef READERS_PREPROCESS_H_
#define READERS_PREPROCESS_H_

#include "../Types.h"
#include "../protobuf/data.pb.h"

#include <vector>
#include <tuple>
#include <string>
#include <google/protobuf/text_format.h>

// Main entry point.
std::string buildCache(std::string scenarioPath,
    Id numPeople, const std::vector<Id> &personPartitionOffsets,
    Id numLocations, const std::vector<Id> &locationPartitionOffsets, int numDays);

// Helper functions.
void buildObjectLookupCache(Id numObjs, const std::vector<Id> &offsets,
  std::string metadataPath, std::string inputPath, std::string outputPath);
void buildActivityCache(Id numPeople, int numDays, Id firstPersonIdx,
  std::string metadataPath, std::string inputPath, std::string outputPath);
int getDay(Time timeInSeconds, Time firstDay);
int getSeconds(Time day, Time firstDay);
std::string getScenarioId(Id numPeople, PartitionId numPeopleChares, Id numLocations,
  PartitionId numLocationChares);
Id getFirstIndex(const loimos::proto::CSVDefinition *metadata, std::string inputPath);
// Emulates C++17 std::filesystem::create_directory
bool create_directory(std::string path, std::string referencePath);

#endif  // READERS_PREPROCESS_H_

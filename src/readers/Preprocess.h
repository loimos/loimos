/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __PREPROCESS_H__
#define __PREPROCESS_H__

#include <tuple>
#include <string>

// Main entry point.
std::tuple<int, int, std::string> buildCache(std::string scenarioPath, int numPeople, int peopleChares, int numLocations, int numLocationChares, int numDays);

// Helper functions.
int buildObjectLookupCache(std::string inputPath, std::string outputPath, int numObjs, int numChares, std::string pathToCsvDefinition);
void buildActivityCache(std::string inputPath, std::string outputPath, int numPeople, int numDays, int firstPersonIdx, std::string pathToCsvDefinition);
int getDay(int timeInSeconds);
std::string getScenarioId(int numPeople, int numPeopleChares, int numLocations,
    int numLocationChares);

#endif //__PREPROCESS_H__

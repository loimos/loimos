/* Copyright 2021 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __PREPROCESS_H__
#define __PREPROCESS_H__

#include <tuple>
#include <string>

// Helper functions.
int getDay(int timeInSeconds);
int buildObjectLookupCache(std::string inputPath, std::string outputPath, int numObjs, int numChares, std::string pathToCsvDefinition);
void buildActivityCache(std::string inputPath, std::string outputPath, int numPeople, int numDays, int firstPersonIdx, std::string pathToCsvDefinition);

// Main entry point.
std::tuple<int, int, std::string> buildCache(std::string scenarioPath, int numPeople, int peopleChares, int numLocations, int numLocationChares, int numDays);

#endif

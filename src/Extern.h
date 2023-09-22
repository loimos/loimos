/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef EXTERN_H_
#define EXTERN_H_

#include "loimos.decl.h"

#include <string>

extern /* readonly */ CProxy_Main mainProxy;
extern /* readonly */ CProxy_People peopleArray;
extern /* readonly */ CProxy_Locations locationsArray;
#ifdef USE_HYPERCOMM
extern /* readonly */ CProxy_Aggregator aggregatorProxy;
#endif
extern /* readonly */ CProxy_DiseaseModel globDiseaseModel;
extern /* readonly */ Id numPeople;
extern /* readonly */ Id numLocations;
extern /* readonly */ PartitionId numPersonPartitions;
extern /* readonly */ PartitionId numLocationPartitions;
extern /* readonly */ int numDays;
extern /* readonly */ int numDaysWithDistinctVisits;
extern /* readonly */ int contactModelType;
extern /* readonly */ bool syntheticRun;

extern /* readonly */ Counter totalVisits;
extern /* readonly */ Counter totalInteractions;
extern /* readonly */ int64_t totalExposures;
extern /* readonly */ int64_t totalExposureDuration;
extern /* readonly */ double simulationStartTime;
extern /* readonly */ double iterationStartTime;

// For real data run.
extern /* readonly */ std::string scenarioPath;
extern /* readonly */ std::string scenarioId;
extern /* readonly */ int maxSimVisitsIdx;
extern /* readonly */ int ageIdx;

// For synthetic run.
extern /* readonly */ Id synPeopleGridWidth;
extern /* readonly */ Id synPeopleGridHeight;
extern /* readonly */ Id synLocationGridWidth;
extern /* readonly */ Id synLocationGridHeight;
extern /* readonly */ Id synLocalLocationGridWidth;
extern /* readonly */ Id synLocalLocationGridHeight;
extern /* readonly */ PartitionId synLocationPartitionGridWidth;
extern /* readonly */ PartitionId synLocationPartitionGridHeight;
extern /* readonly */ int averageDegreeOfVisit;

// Intervention
extern /* readonly */ bool interventionStategy;

#endif  // EXTERN_H_

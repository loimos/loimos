/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __DEFS_H__
#define __DEFS_H__

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
// TODO: change to commandline argument (and rename to reflect flexibility)
#define DAYS_IN_WEEK 7

// Indices of attribute columns in the appropriate csvs
#define AGE_CSV_INDEX 0
#define SIMULTANEOUS_MAX_VISITS_CSV_INDEX 1

// Data loading
#define EMPTY_VISIT_SCHEDULE 0xFFFFFFFF
#define CSV_DELIM ','

#define PERCENTAGE_OF_SEEDING_LOCATIONS 0.001
#define INITIAL_INFECTIOUS_PROBABILITY 0.01
#define DAYS_TO_SEED_INFECTION 3
#define INITIAL_INFECTIONS_PER_DAY 2
#define INITIAL_INFECTIONS (INITIAL_INFECTIONS_PER_DAY * DAYS_TO_SEED_INFECTION)

extern /* readonly */ CProxy_Main mainProxy;
extern /* readonly */ CProxy_People peopleArray;
extern /* readonly */ CProxy_Locations locationsArray;
#ifdef USE_HYPERCOMM
extern /* readonly */ CProxy_Aggregator aggregatorProxy;
#endif
extern /* readonly */ CProxy_DiseaseModel globDiseaseModel;
extern /* readonly */ int numPeople;
extern /* readonly */ int numLocations;
extern /* readonly */ int numPeoplePartitions;
extern /* readonly */ int numLocationPartitions;
extern /* readonly */ int numDays;
extern /* readonly */ int numDaysWithRealData;
extern /* readonly */ bool syntheticRun;

extern /* readonly */ uint64_t totalVisits;
extern /* readonly */ double simulationStartTime;
extern /* readonly */ double iterationStartTime;

// For real data run.
extern /* readonly */ std::string scenarioPath;
extern /* readonly */ std::string scenarioId;
extern /* readonly */ int firstPersonIdx;
extern /* readonly */ int firstLocationIdx;

// For synthetic run.
extern /* readonly */ int synPeopleGridWidth;
extern /* readonly */ int synPeopleGridHeight;
extern /* readonly */ int synLocationGridWidth;
extern /* readonly */ int synLocationGridHeight;
extern /* readonly */ int synLocalLocationGridWidth;
extern /* readonly */ int synLocalLocationGridHeight;
extern /* readonly */ int synLocationPartitionGridWidth;
extern /* readonly */ int synLocationPartitionGridHeight;
extern /* readonly */ int averageDegreeOfVisit;

// Intervention
extern /* readonly */ bool interventionStategy;

int getNumElementsPerPartition(int numElements, int numPartitions);

int getNumLocalElements(int numElements, int numPartitions, int partitionIndex);

int getPartitionIndex(int globalIndex, int numElements, int numPartitions, int offset);

int getLocalIndex(int globalIndex, int numElements, int numPartitions, int offset);

int getGlobalIndex(int localIndex, int partitionIndex, int numElements, int numPartitions, int offset);

#endif // __DEFS_H__

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

// CSV Order
#define AGE_CSV_LOC 0

// Data loading
#define EMPTY_VISIT_SCHEDULE 0xFFFFFFFF
#define CSV_DELIM ','

#define INITIAL_INFECTIOUS_PROBABILITY 0.05

extern /* readonly */ CProxy_Main mainProxy;
extern /* readonly */ CProxy_People peopleArray;
extern /* readonly */ CProxy_Locations locationsArray;
extern /* readonly */ CProxy_DiseaseModel globDiseaseModel;
extern /* readonly */ int numPeople;
extern /* readonly */ int numLocations;
extern /* readonly */ int numPeoplePartitions;
extern /* readonly */ int numLocationPartitions;
extern /* readonly */ int numDays;
extern /* readonly */ bool syntheticRun;
extern /* readonly */ int firstPersonIdx;
extern /* readonly */ int firstLocationIdx;
extern /* readonly */ std::string scenarioPath;
extern /* readonly */ std::string scenarioId;
extern /* readonly */ double simulationStartTime;

int getNumElementsPerPartition(int numElements, int numPartitions);

int getNumLocalElements(int numElements, int numPartitions, int partitionIndex);

int getPartitionIndex(int globalIndex, int numElements, int numPartitions, int offset);

int getLocalIndex(int globalIndex, int numElements, int numPartitions, int offset);

int getGlobalIndex(int localIndex, int partitionIndex, int numElements, int numPartitions, int offset);

#endif // __DEFS_H__

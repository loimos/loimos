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

// TODO THIS NEED TO BE DYNAMIC (including in indexing)
#define PERSON_OFFSET 5586585
#define LOCATION_OFFSET 0

// Time
using Time = int32_t;
const Time DAY_LENGTH = 3600 * 24;

#define INITIAL_INFECTIOUS_PROBABILITY 0.10

extern /* readonly */ CProxy_Main mainProxy;
extern /* readonly */ CProxy_People peopleArray;
extern /* readonly */ CProxy_Locations locationsArray;
extern /* readonly */ CProxy_DiseaseModel globDiseaseModel;
extern /* readonly */ int numPeople;
extern /* readonly */ int numLocations;
extern /* readonly */ int numPeoplePartitions;
extern /* readonly */ int numLocationPartitions;
extern /* readonly */ int numDays;
extern /* readonly */ std::string scenarioPath;

int getNumElementsPerPartition(int numElements, int numPartitions);

int getNumLocalElements(int numElements, int numPartitions, int partitionIndex);

int getPartitionIndex(int globalIndex, int numElements, int numPartitions, int offset);

int getLocalIndex(int globalIndex, int numElements, int numPartitions, int offset);

int getGlobalIndex(int localIndex, int partitionIndex, int numElements, int numPartitions, int offset);

#endif // __DEFS_H__

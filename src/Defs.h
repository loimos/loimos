/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __DEFS_H__
#define __DEFS_H__

#define HEALTHY 0
#define INFECTED 1

#define INFECTION_PROBABILITY 0.05

extern /* readonly */ CProxy_Main mainProxy;
extern /* readonly */ CProxy_People peopleArray;
extern /* readonly */ CProxy_Locations locationsArray;
extern /* readonly */ int numPeople;
extern /* readonly */ int numLocations;
extern /* readonly */ int numPeoplePartitions;
extern /* readonly */ int numLocationPartitions;
extern /* readonly */ int numDays;

int getElemsPerCont(int numElements, int numContainers);

int getNumLocalElems(int numElements, int numContainers, int containerIndex);

int getContainerIndex(int globalIndex, int numElements, int numContainers);

int getLocalIndex(int globalIndex, int numElements, int numContainers);

int getGlobalIndex(int localIndex, int contIndex, int numElements, int numContainers);

#endif // __DEFS_H__

/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "Locations.h"
#include "Defs.h"
#include <algorithm>

Locations::Locations() {
  // getting number of locations assigned to this chare
  numLocalLocations = getNumLocalElements(numLocations, numLocationPartitions, thisIndex);
  locState.resize(numLocalLocations);
  generator.seed(thisIndex);
  MAX_RANDOM_VALUE = (float)generator.max();
}

void Locations::ReceiveVisitMessages(int personIdx, int locationIdx) {
  // adding person to location visit list
  int localLocIdx = getLocalIndex(locationIdx, numLocations, numLocationPartitions);
  locState[localLocIdx].push_back(personIdx);
  // CkPrintf("Location %d localIdx %d visited by person %d\n",locationIdx,localLocIdx,personIdx);
}

void Locations::ComputeInteractions() {
  int peopleSubsetIdx;
  int tmp=0, globalIdx;
  char state;
  float value;
  // traverses list of locations
  for(std::vector<std::vector<int> >::iterator locIter = locState.begin() ; locIter != locState.end(); ++locIter) {
    // sorting set of people to guarantee deterministic behavior
    sort(locIter->begin(),locIter->end());
    globalIdx = getGlobalIndex(tmp,thisIndex,numLocations,numLocationPartitions);
    for(std::vector<int>::iterator it = locIter->begin() ; it != locIter->end(); ++it) {
      // randomly selecting people to get infected
      value = (float)generator();
      //CkPrintf("Partition %d - Location %d - Person %d - Value %f\n", thisIndex, globalIdx, *it, value);
      if(value/MAX_RANDOM_VALUE < INFECTION_PROBABILITY)
        state = INFECTED;
      else
        state = HEALTHY;
      peopleSubsetIdx = getPartitionIndex(*it, numPeople, numPeoplePartitions);
      peopleArray[peopleSubsetIdx].ReceiveInfections(*it,state);
    }
    tmp++;
  }
}


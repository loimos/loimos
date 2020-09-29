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
  visitors.resize(numLocalLocations);
  locationState.resize(numLocalLocations, SUSCEPTIBLE);
  generator.seed(thisIndex);
  MAX_RANDOM_VALUE = (float)generator.max();
}

void Locations::ReceiveVisitMessages(VisitMessage visit_msg) {
  // adding person to location visit list
  int localLocIdx = getLocalIndex(visit_msg.locationIdx, numLocations,
      numLocationPartitions);
  visitors[localLocIdx].push_back(std::pair<int,char>(visit_msg.personIdx,
        visit_msg.personState));
  if (visit_msg.personState == INFECTIOUS) {
    locationState[localLocIdx] = INFECTIOUS;
  }
  // CkPrintf("Location %d localIdx %d visited by person %d\n", locationIdx, localLocIdx, personIdx);
}

void Locations::ComputeInteractions() {
  int peopleSubsetIdx;
  int cont=0, globalIdx;
  float value;
  // traverses list of locations
  for(std::vector<std::vector<std::pair<int,char> > >::iterator locIter = visitors.begin() ; locIter != visitors.end(); ++locIter) {
    if(locationState[cont] == INFECTIOUS) {
      // sorting set of people to guarantee deterministic behavior
      sort(locIter->begin(), locIter->end());
      globalIdx = getGlobalIndex(cont, thisIndex, numLocations, numLocationPartitions);
      for(std::vector<std::pair<int,char> >::iterator visitor = locIter->begin() ; visitor != locIter->end(); ++visitor) {
        // randomly selecting people to get infected
        value = (float)generator();
        // CkPrintf("Partition %d - Location %d - Person %d - Value %f\n", thisIndex, globalIdx, *visitor, value);
        if(visitor->second == SUSCEPTIBLE && value/MAX_RANDOM_VALUE < INFECTION_PROBABILITY) {
          peopleSubsetIdx = getPartitionIndex(visitor->first, numPeople, numPeoplePartitions);
          peopleArray[peopleSubsetIdx].ReceiveInfections(visitor->first);
        }
      }
    }
    // cleaning the visits to this location
    locIter->clear();
    cont++;
  }
  // cleaning state of all locations
  locationState.resize(numLocalLocations, SUSCEPTIBLE);
}

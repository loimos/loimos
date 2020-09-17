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
  locationPeople.resize(numLocalLocations);
  locationState.resize(numLocalLocations, SUSCEPTIBLE);
  generator.seed(thisIndex);
  MAX_RANDOM_VALUE = (float)generator.max();
}

void Locations::ReceiveVisitMessages(int personIdx, char personState, int locationIdx) {
  // adding person to location visit list
  int localLocIdx = getLocalIndex(locationIdx, numLocations, numLocationPartitions);
  locationPeople[localLocIdx].push_back(std::pair<int,char>(personIdx, personState));
  if(personState == INFECTIOUS) locationState[localLocIdx] = INFECTIOUS;
  // CkPrintf("Location %d localIdx %d visited by person %d\n", locationIdx, localLocIdx, personIdx);
}

void Locations::ComputeInteractions() {
  int peopleSubsetIdx;
  int cont=0, globalIdx;
  float value;
  // traverses list of locations
  for(std::vector<std::vector<std::pair<int,char> > >::iterator locIter = locationPeople.begin() ; locIter != locationPeople.end(); ++locIter) {
    if(locationState[cont] == INFECTIOUS) {
      // sorting set of people to guarantee deterministic behavior
      sort(locIter->begin(), locIter->end());
      globalIdx = getGlobalIndex(cont, thisIndex, numLocations, numLocationPartitions);
      for(std::vector<std::pair<int,char> >::iterator it = locIter->begin() ; it != locIter->end(); ++it) {
        // randomly selecting people to get infected
        value = (float)generator();
        // CkPrintf("Partition %d - Location %d - Person %d - Value %f\n", thisIndex, globalIdx, *it, value);
        if(it->second == SUSCEPTIBLE && value/MAX_RANDOM_VALUE < INFECTION_PROBABILITY) {
          peopleSubsetIdx = getPartitionIndex(it->first, numPeople, numPeoplePartitions);
          peopleArray[peopleSubsetIdx].ReceiveInfections(it->first);
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


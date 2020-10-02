/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "Locations.h"
#include "DiseaseModel.h"
#include "Location.h"
#include "Event.h"
#include "Defs.h"

#include <algorithm>
#include <functional>
#include <queue>
#include <stdio.h>

Locations::Locations() {
  // getting number of locations assigned to this chare
  numLocalLocations = getNumLocalElements(
    numLocations,
    numLocationPartitions,
    thisIndex
  );
  // Init disease states.
  diseaseModel = globDiseaseModel.ckLocalBranch();
  locationState.resize(numLocalLocations, diseaseModel->getHealthyState());
  
  // Seed random number generator via branch ID for reproducibility.
  locations.resize(numLocalLocations);

  generator.seed(thisIndex);
  MAX_RANDOM_VALUE = (float) generator.max();
}

void Locations::ReceiveVisitMessages(
  int locationIdx,
  int personIdx,
  char personState,
  int visitStart,
  int visitEnd
) {
  // adding person to location visit list
  int localLocIdx = getLocalIndex(
    locationIdx,
    numLocations,
    numLocationPartitions
  );

  Event arrival { personIdx, personState, visitStart, ARRIVAL };
  Event departure { personIdx, personState, visitEnd, DEPARTURE };

  locations[localLocIdx].addEvent(arrival);
  locations[localLocIdx].addEvent(departure);

  //CkPrintf(
  //  "Location %d localIdx %d visited by person %d\n",
  //  locationIdx,
  //  localLocIdx,
  //  personIdx
  //);
}

void Locations::ComputeInteractions() {
  int peopleSubsetIdx;
  //int cont = 0, globalIdx;
  char state;
  float value;

  // traverses list of locations
  for (auto loc : locations) {
    //globalIdx = getGlobalIndex(
    //  cont,
    //  thisIndex,
    //  numLocations,
    //  numLocationPartitions
    //);
    
    std::unordered_set<int> justInfected = loc.processEvents(generator);
    for (int personIdx : justInfected) {
      infect(personIdx);
    }
    justInfected.empty();
  }
  
  // cleaning state of all locations
}

// Simple helper function which infects a given person with a given
// probability
inline void Locations::infect(int personIdx) {
  int peoplePartitionIdx = getPartitionIndex(
    personIdx,
    numPeople,
    numPeoplePartitions
  );

  peopleArray[peoplePartitionIdx].ReceiveInfections(personIdx);
  printf(
    "sending infection message to person %d in partition %d\r\n",
    personIdx,
    peoplePartitionIdx
  );
}


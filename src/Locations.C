/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "Locations.h"
#include "DiseaseModel.h"
#include "Defs.h"
#include <algorithm>

Locations::Locations() {
  // getting number of locations assigned to this chare
  numLocalLocations = getNumLocalElements(
    numLocations,
    numLocationPartitions,
    thisIndex
  );
  visitors.resize(numLocalLocations);
  
  // Init disease states.
  diseaseModel = globDiseaseModel.ckLocalBranch();
  locationState.resize(numLocalLocations, diseaseModel->getHealthyState());
  
  // Seed random number generator via branch ID for reproducibility.
  generator.seed(thisIndex);
  MAX_RANDOM_VALUE = (float) generator.max();
}

void Locations::ReceiveVisitMessages(
  int personIdx,
  char personState,
  int locationIdx,
  int visitStart,
  int visitEnd
) {
  // adding person to location visit list
  int localLocIdx = getLocalIndex(
    locationIdx,
    numLocations,
    numLocationPartitions
  );
  
  if(personState == INFECTIOUS)
    locationState[localLocIdx] = INFECTIOUS;
  
  visitors[localLocIdx].push_back(
    std::pair<int,char>(personIdx, personState)
  );
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
  for (auto locIter : visitors) {
    // sorting set of people to guarantee deterministic behavior
    sort(locIter.begin(), locIter.end());
    //globalIdx = getGlobalIndex(
    //  cont,
    //  thisIndex,
    //  numLocations,
    //  numLocationPartitions
    //);

    for (auto it : locIter) {
      // randomly selecting people to get infected
      value = (float) generator();
      
      //CkPrintf(
      //  "Partition %d - Location %d - Person %d - Value %f\n",
      //  thisIndex,
      //  globalIdx,
      //  *it,
      //  value
      //);
      
      if (value/MAX_RANDOM_VALUE < INFECTION_PROBABILITY) {
        state = INFECTED;
      } else {
        state = HEALTHY;
      }
      
      peopleSubsetIdx = getPartitionIndex(
        it,
        numPeople,
        numPeoplePartitions
      );
      peopleArray[peopleSubsetIdx].ReceiveInfections(it, state);
    }

    //cont++;
  }
  
  // cleaning state of all locations
  locationState.resize(numLocalLocations, diseaseModel->getHealthyState());
}


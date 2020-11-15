/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "Locations.h"
#include "Location.h"
#include "Event.h"
#include "DiseaseModel.h"
#include "Location.h"
#include "Event.h"
#include "Defs.h"
#include "Attributes.h"

#include <algorithm>
#include <functional>
#include <queue>
#include <stdio.h>
#include <iostream>
#include <fstream>

Locations::Locations() {
  // getting number of locations assigned to this chare
  numLocalLocations = getNumLocalElements(
    numLocations,
    numLocationPartitions,
    thisIndex
  );
  locations.resize(numLocalLocations);
  
  // Init disease states.
  diseaseModel = globDiseaseModel.ckLocalBranch();

  // Load in location information
  int startingLineIndex = getGlobalIndex(0, thisIndex, numLocations, numLocationPartitions, LOCATION_OFFSET) - LOCATION_OFFSET;
  int endingLineIndex = startingLineIndex + numLocalLocations;
  std::string line;

  std::ifstream f("sample_input/locations.csv");
  if (!f) {
    CkAbort("Could not open person data input.");
  }
  
  // TODO (iancostello): build an index at preprocessing and seek. 
  for (int i = 0; i <= startingLineIndex; i++) {
    std::getline(f, line);
  }
    
  for (int i = 0; i < numLocalLocations; i++) {
    std::getline(f, line);
    loadLocationFromCSV(i, &line);
  }

  // Seed random number generator via branch ID for reproducibility.
  generator.seed(thisIndex);
}

void Locations::loadLocationFromCSV(int locationIdx, std::string *data) {
  int attr_index = 0;
  int left_comma = 0;

  // TODO general purpose processor.
  for (int c = 0; c < data->length(); c++) {
    if (data->at(c) == ',') {
      // Completed attribute.
      std::string sub_str = data->substr(left_comma, c);

      //TODO HANDLE THESE ATTRIBUTES WITH PROTOBUF
      if (attr_index == 0) {
        locations[locationIdx].unique_id = std::stoi(sub_str);
      }

      left_comma = c + 1;
      attr_index += 1;
    }
  }
}


void Locations::ReceiveVisitMessages(
  int locationIdx,
  int personIdx,
  int personState,
  int visitStart,
  int visitEnd
) {
  // adding person to location visit list
  int localLocIdx = getLocalIndex(
    locationIdx,
    numLocations,
    numLocationPartitions,
    LOCATION_OFFSET
  );

  // CkPrintf("%d: Received visit to %d (loc %d) from person %d\n", thisIndex, locationIdx, localLocIdx, personIdx);

  // Wrap vist info...
  Event arrival { personIdx, personState, visitStart, ARRIVAL };
  Event departure { personIdx, personState, visitEnd, DEPARTURE };

  // ...and queue it up at the appropriate location
  locations[localLocIdx].addEvent(arrival);
  locations[localLocIdx].addEvent(departure);
}

void Locations::ComputeInteractions() {
  int peopleSubsetIdx;
  int state;
  float value;

  // traverses list of locations
  for (auto loc : locations) {
    std::unordered_set<int> justInfected =
      loc.processEvents(&generator, diseaseModel);
    
    for (int personIdx : justInfected) {
      infect(personIdx);
    }
    justInfected.empty();
  }
}

// Simple helper function which infects a given person with a given
// probability (we handle this here rather since this is a chare class,
// and so we have access to peopleArray and the like)
inline void Locations::infect(int personIdx) {
  int peoplePartitionIdx = getPartitionIndex(
    personIdx,
    numPeople,
    numPeoplePartitions,
    PERSON_OFFSET
  );

  peopleArray[peoplePartitionIdx].ReceiveInfections(personIdx);
  /*
  CkPrintf(
    "sending infection message to person %d in partition %d\r\n",
    personIdx,
    peoplePartitionIdx
  );
  */
}

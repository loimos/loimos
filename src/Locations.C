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
#include "ContactModel.h"
#include "Location.h"
#include "Event.h"
#include "Defs.h"
#include "Attributes.h"

#include <algorithm>
#include <queue>
#include <stdio.h>
#include <iostream>
#include <fstream>

Locations::Locations() {
  // Getting number of locations assigned to this chare
  numLocalLocations = getNumLocalElements(
    numLocations,
    numLocationPartitions,
    thisIndex
  );
  
  // Init disease states
  diseaseModel = globDiseaseModel.ckLocalBranch();

  // Init local 
  int numAttributesPerLocation = 
    DataReader<Person>::getNonZeroAttributes(diseaseModel->locationDef);
  for (int p = 0; p < numLocalLocations; p++) {
    locations.push_back(new Location(numAttributesPerLocation));
  }

  // Load in location information
  int startingLineIndex = getGlobalIndex(0, thisIndex, numLocations, numLocationPartitions, LOCATION_OFFSET) - LOCATION_OFFSET;
  int endingLineIndex = startingLineIndex + numLocalLocations;
  std::string line;

  std::ifstream f(scenarioPath + "locations.csv");
  if (!f) {
    CkAbort("Could not open person data input.");
  }
  
  // TODO (iancostello): build an index at preprocessing and seek. 
  for (int i = 0; i <= startingLineIndex; i++) {
    std::getline(f, line);
  }
  DataReader<Location *>::readData(&f, diseaseModel->locationDef, &locations);
  f.close();

  // Seed random number generator via branch ID for reproducibility.
  generator.seed(thisIndex);
  // Init contact model
  contactModel = new ContactModel();
  contactModel->setGenerator(&generator);
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
  Event arrival { ARRIVAL, personIdx, personState, visitStart };
  Event departure { DEPARTURE, personIdx, personState, visitEnd };
  Event::pair(&arrival, &departure);

  // ...and queue it up at the appropriate location
  locations[localLocIdx]->addEvent(arrival);
  locations[localLocIdx]->addEvent(departure);
}

void Locations::ComputeInteractions() {
  // traverses list of locations
  for (Location loc : locations) {
    loc.processEvents(diseaseModel, contactModel);
  }
}

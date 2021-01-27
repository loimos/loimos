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
#include "Person.h"
#include "Event.h"
#include "Defs.h"
#include "data/DataReader.h"

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

  // Load application data
  loadLocationData();

  // Seed random number generator via branch ID for reproducibility.
  generator.seed(thisIndex);
}

void Locations::loadLocationData() {
  // Init local.
  int numAttributesPerLocation = 
    DataReader<Person>::getNonZeroAttributes(diseaseModel->locationDef);
  for (int p = 0; p < numLocalLocations; p++) {
    locations.push_back(new Location(numAttributesPerLocation));
  }

  // Load in location information.
  int startingLineIndex = getGlobalIndex(0, thisIndex, numLocations, numLocationPartitions, firstLocationIdx) - firstLocationIdx;
  int endingLineIndex = startingLineIndex + numLocalLocations;
  std::string line;

  std::ifstream locationData(scenarioPath + "locations.csv");
  std::ifstream locationCache(scenarioPath + scenarioId + "_locations.cache");
  if (!locationData || !locationCache) {
    CkAbort("Could not open person data input.");
  }
  
  // Find starting line for our data through location cache.
  locationCache.seekg(thisIndex * sizeof(uint32_t));
  uint32_t locationOffset;
  locationCache.read((char *) &locationOffset, sizeof(uint32_t));
  locationData.seekg(locationOffset);

  // Read in our location data.
  DataReader<Location *>::readData(&locationData, diseaseModel->locationDef, &locations);
  locationData.close();
  locationCache.close();

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
    firstLocationIdx
  );

  CkPrintf("%d: Received visit to %d (loc %d) from person %d (%d)\n", thisIndex, locationIdx, localLocIdx, personIdx, firstLocationIdx);

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
  for (Location *loc : locations) {
    loc->processEvents(diseaseModel, contactModel);
  }
}

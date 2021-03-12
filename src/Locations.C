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
#include "contact_model/ContactModel.h"
#include "Location.h"
#include "Event.h"
#include "Defs.h"
#include "readers/DataReader.h"
#include "Person.h"

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
  if (syntheticRun) {
    Location tmp { 0 };
    locations.resize(numLocalLocations, tmp);
  } else {
    Location tmp { 0 };
    locations.resize(numLocalLocations, tmp);
    loadLocationData();
  }

  // Seed random number generator via branch ID for reproducibility
  generator.seed(thisIndex);
  // Init contact model
  contactModel = new ContactModel();
  contactModel->setGenerator(&generator);
}

void Locations::loadLocationData() {
  // Init local.
  int numAttributesPerLocation = 
    DataReader<Person>::getNonZeroAttributes(diseaseModel->locationDef);
  locations.reserve(numLocalLocations);
  for (int p = 0; p < numLocalLocations; p++) {
    locations.emplace_back(numAttributesPerLocation);
  }

  // Load in location information.
  int startingLineIndex = getGlobalIndex(
    0,
    thisIndex,
    numLocations,
    numLocationPartitions,
    firstLocationIdx
  ) - firstLocationIdx;
  int endingLineIndex = startingLineIndex + numLocalLocations;
  std::string line;

  std::ifstream locationData(scenarioPath + "locations.csv");
  std::ifstream locationCache(scenarioPath + scenarioId + "_locations.cache", std::ios_base::binary);
  if (!locationData || !locationCache) {
    CkAbort("Could not open person data input.");
  }
  
  // Find starting line for our data through location cache.
  locationCache.seekg(thisIndex * sizeof(uint32_t));
  uint32_t locationOffset;
  locationCache.read((char *) &locationOffset, sizeof(uint32_t));
  locationData.seekg(locationOffset);

  // Read in our location data.
  DataReader<Location>::readData(
      &locationData,
      diseaseModel->locationDef,
      &locations
  );
  locationData.close();
  locationCache.close();

  // Seed random number generator via branch ID for reproducibility.
  generator.seed(thisIndex);
  
  // Init contact model
  contactModel = new ContactModel();
  contactModel->setGenerator(&generator);

  // Let contact model add any attributes it needs to the locations
  for (Location &location: locations) {
    contactModel->computeLocationValues(location);
  }
}

void Locations::ReceiveVisitMessages(VisitMessage visitMsg) {
  // adding person to location visit list
  int localLocIdx = getLocalIndex(
    visitMsg.locationIdx,
    numLocations,
    numLocationPartitions,
    firstLocationIdx
  );

  // Wrap vist info...
  Event arrival { ARRIVAL, visitMsg.personIdx, visitMsg.personState, visitMsg.visitStart };
  Event departure { DEPARTURE, visitMsg.personIdx, visitMsg.personState, visitMsg.visitEnd };
  Event::pair(&arrival, &departure);

  // ...and queue it up at the appropriate location
  locations[localLocIdx].addEvent(arrival);
  locations[localLocIdx].addEvent(departure);
}

void Locations::ComputeInteractions() {
  // traverses list of locations
  for (Location loc : locations) {
    loc.processEvents(diseaseModel, contactModel);
  }
}

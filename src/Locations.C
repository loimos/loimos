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
#include "Person.h"
#include "readers/DataInterfaceMessage.h"

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
  locationsInitialized = 0;
  locations.reserve(numLocalLocations);
  int firstIdx = thisIndex * getNumLocalElements(numLocations, numLocationPartitions, 0);
  int numAttributes = syntheticRun ? 0 : DataLoader::getNumberOfDataAttributes(diseaseModel->locationDef);
  for (int p = 0; p < numLocalLocations; p++) {
    locations.emplace_back(numAttributes, firstIdx + p, &generator);
  }

  // Seed random number generator via branch ID for reproducibility
  generator.seed(thisIndex);

  // Init contact model
  contactModel = createContactModel();
  contactModel->setGenerator(&generator);
}

void Locations::ReceiveLocationSetup(DataInterfaceMessage *msg) {
  // Copy read data into next person and increment.
  locations[locationsInitialized].uniqueId = msg->uniqueId;
  for (int i = 0; i < msg->numDataAttributes; i++) {
    locations[locationsInitialized].setField(i, msg->dataAttributes[i]);
  }
  contactModel->computeLocationValues(locations[locationsInitialized]);
  locationsInitialized += 1;
  assert(locationsInitialized <= numLocalLocations);
}

void Locations::ReceiveVisitMessages(VisitMessage visitMsg) {
  
  // adding person to location visit list
  int localLocIdx = getLocalIndex(
    visitMsg.locationIdx,
    numLocations,
    numLocationPartitions,
    firstLocationIdx
  );

  //CkPrintf("Visiting location %d (%d of %d locally)\r\n",
  //  visitMsg.locationIdx, localLocIdx, numLocalLocations);

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
  for (Location &loc : locations) {
    loc.processEvents(diseaseModel, contactModel);
  }
}


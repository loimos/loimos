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

#include <algorithm>
#include <queue>
#include <stdio.h>

Locations::Locations() {
  // Getting number of locations assigned to this chare
  numLocalLocations = getNumLocalElements(
    numLocations,
    numLocationPartitions,
    thisIndex
  );
  locations.resize(numLocalLocations);
  
  // Init disease states
  diseaseModel = globDiseaseModel.ckLocalBranch();
  // Seed random number generator via branch ID for reproducibility
  generator.seed(thisIndex);
  // Init contact model
  contactModel = new ContactModel();
  contactModel->setGenerator(&generator);
}

void Locations::ReceiveVisitMessages(VisitMessage visitMsg) {
  // adding person to location visit list
  int localLocIdx = getLocalIndex(
    visitMsg.locationIdx,
    numLocations,
    numLocationPartitions
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

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
#include <functional>
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
    numLocationPartitions
  );

  // Wrap vist info...
  Event arrival { ARRIVAL, personIdx, personState, visitStart };
  Event departure { DEPARTURE, personIdx, personState, visitEnd };
  Event::pair(&arrival, &departure);

  // ...and queue it up at the appropriate location
  locations[localLocIdx].addEvent(arrival);
  locations[localLocIdx].addEvent(departure);
}

void Locations::ComputeInteractions() {
  int peopleSubsetIdx;
  int state;
  float value;

  // traverses list of locations
  for (Location loc : locations) {
    loc.processEvents(diseaseModel);
  }
}

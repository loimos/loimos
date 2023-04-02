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
#include "pup_stl.h"

#include <algorithm>
#include <queue>
#include <stdio.h>
#include <iostream>
#include <fstream>

Locations::Locations() {
  day = 0;

  //Must be set to true to make AtSync work
  usesAtSync = true;

  // Getting number of locations assigned to this chare
  numLocalLocations = getNumLocalElements(
    numLocations,
    numLocationPartitions,
    thisIndex
  );

  // Init disease states
  diseaseModel = globDiseaseModel.ckLocalBranch();

  // Seed random number generator via branch ID for reproducibility
  generator.seed(time(NULL));
  // generator.seed(thisIndex);

  // Init contact model
  contactModel = createContactModel();
  contactModel->setGenerator(&generator);

  // Load application data
  if (syntheticRun) {
    locations.reserve(numLocalLocations);
    int firstIdx = thisIndex * getNumLocalElements(numLocations, numLocationPartitions, 0);
    for (int p = 0; p < numLocalLocations; p++) {
      locations.emplace_back(0, firstIdx + p, &generator, diseaseModel);
    }
  } else {
    loadLocationData();
  }

  // Notify Main
  #ifdef USE_HYPERCOMM
  contribute(CkCallback(CkReductionTarget(Main, CharesCreated), mainProxy));
  #endif
}

Locations::Locations(CkMigrateMessage *msg) {};

void Locations::loadLocationData() {
  double startTime = CkWallTimer();

  // Init local.
  int numAttributesPerLocation =
    DataReader<Person>::getNonZeroAttributes(diseaseModel->locationDef);
  locations.reserve(numLocalLocations);
  int firstIdx = thisIndex * getNumElementsPerPartition(numLocations,
      numLocationPartitions);
  for (int p = 0; p < numLocalLocations; p++) {
    locations.emplace_back(numAttributesPerLocation, firstIdx + p, &generator, diseaseModel);
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
  locationCache.seekg(thisIndex * sizeof(uint64_t));
  uint64_t locationOffset;
  locationCache.read((char *) &locationOffset, sizeof(uint64_t));
  locationData.seekg(locationOffset);

  // Read in our location data.
  DataReader<Location>::readData(
      &locationData,
      diseaseModel->locationDef,
      &locations
  );
  locationData.close();
  locationCache.close();

  // Let contact model add any attributes it needs to the locations
  for (Location &location: locations) {
    contactModel->computeLocationValues(location);
  }

#if ENABLE_DEBUG >= DEBUG_PER_CHARE
  CkPrintf("  Chare %d took %f s to load locations\n", thisIndex,
      CkWallTimer() - startTime);
#endif
}

void Locations::pup(PUP::er &p) {
  p | numLocalLocations;
  p | locations;
  p | generator;
  p | day;

  if (p.isUnpacking()) {
    diseaseModel = globDiseaseModel.ckLocalBranch();
    contactModel = createContactModel();
    contactModel->setGenerator(&generator);

    for (Location &loc: locations) {
      loc.setGenerator(&generator);
    }
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
  int numVisits = 0;
  for (Location &loc : locations) {
    numVisits += loc.events.size() / 2;
    loc.processEvents(diseaseModel, contactModel);
  }

#if ENABLE_DEBUG >= DEBUG_PER_CHARE
  if (0 == day) {
    CkPrintf("    Process %d, thread %d: %d visits, %d locations\n",
      CkMyNode(), CkMyPe(), numVisits, (int) locations.size());
  }
#endif

  day++;
}

#ifdef ENABLE_LB
void Locations::ResumeFromSync() {
#if ENABLE_DEBUG >= DEBUG_PER_CHARE
  CkPrintf("\tDone load balancing on location chare %d\n", thisIndex);
#endif

  CkCallback cb(CkReductionTarget(Main, locationsLBComplete), mainProxy);
  contribute(cb);
}
#endif // ENABLE_LB

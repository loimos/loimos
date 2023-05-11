/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "Types.h"
#include "Locations.h"
#include "Location.h"
#include "Event.h"
#include "DiseaseModel.h"
#include "contact_model/ContactModel.h"
#include "Location.h"
#include "Person.h"
#include "Event.h"
#include "Extern.h"
#include "Defs.h"
#include "readers/Preprocess.h"
#include "readers/DataReader.h"
#include "pup_stl.h"

#include <algorithm>
#include <queue>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>

Locations::Locations(std::string scenarioPath) {
  day = 0;

  // Must be set to true to make AtSync work
  usesAtSync = true;

  // Getting number of locations assigned to this chare
  numLocalLocations = getNumLocalElements(
    numLocations,
    numLocationPartitions,
    thisIndex);

  firstLocalLocationIdx = getFirstIndex(thisIndex, numLocations,
      numLocationPartitions, firstLocationIdx);
#if ENABLE_DEBUG >= DEBUG_PER_CHARE
  CkPrintf("  Chare %d has %d locs (%d-%d)\n",
      thisIndex, numLocalLocations, firstLocalLocationIdx,
      firstLocalLocationIdx + numLocalLocations - 1);
#endif

  // Init disease states
  diseaseModel = globDiseaseModel.ckLocalBranch();

  // Seed random number generator via branch ID for reproducibility
  generator.seed(time(NULL));
  // generator.seed(thisIndex);

  // Init contact model
  contactModel = createContactModel();
  contactModel->setGenerator(&generator);

  // Load application data
  locations.reserve(numLocalLocations);
  if (syntheticRun) {
    for (int i = 0; i < numLocalLocations; i++) {
      locations.emplace_back(0, firstLocalLocationIdx + i, &generator,
          diseaseModel);
    }
  } else {
    loadLocationData(scenarioPath);
  }

  // Notify Main
  #ifdef USE_HYPERCOMM
  contribute(CkCallback(CkReductionTarget(Main, CharesCreated), mainProxy));
  #endif
}

Locations::Locations(CkMigrateMessage *msg) {}

void Locations::loadLocationData(std::string scenarioPath) {
  double startTime = CkWallTimer();

  // Init local.
  int numAttributesPerLocation =
    DataReader<Person>::getNonZeroAttributes(diseaseModel->locationDef);
  for (int p = 0; p < numLocalLocations; p++) {
    locations.emplace_back(numAttributesPerLocation, firstLocalLocationIdx + p,
        &generator, diseaseModel);
  }

  // Load in location information.
  int startingLineIndex = getGlobalIndex(
    0,
    thisIndex,
    numLocations,
    numLocationPartitions,
    firstLocationIdx) - firstLocationIdx;
  int endingLineIndex = startingLineIndex + numLocalLocations;
  std::string line;

  std::string scenarioId = getScenarioId(numPeople, numPeoplePartitions,
    numLocations, numLocationPartitions);
  std::ifstream locationData(scenarioPath + "locations.csv");
  std::ifstream locationCache(scenarioPath + scenarioId
    + "_locations.cache", std::ios_base::binary);
  if (!locationData || !locationCache) {
    CkAbort("Could not open person data input.");
  }

  // Find starting line for our data through location cache.
  locationCache.seekg(thisIndex * sizeof(uint64_t));
  uint64_t locationOffset;
  locationCache.read(reinterpret_cast<char *>(&locationOffset), sizeof(uint64_t));
  locationData.seekg(locationOffset);

  // Read in our location data.
  DataReader<Location>::readData(
      &locationData,
      diseaseModel->locationDef,
      &locations);
  locationData.close();
  locationCache.close();

  // Let contact model add any attributes it needs to the locations
  for (Location &location : locations) {
    contactModel->computeLocationValues(&location);
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

    for (Location &loc : locations) {
      loc.setGenerator(&generator);
    }
  }
}

void Locations::ReceiveVisitMessages(VisitMessage visitMsg) {
  // adding person to location visit list
  int localLocIdx = getLocalIndex(
    visitMsg.locationIdx,
    thisIndex,
    numLocations,
    numLocationPartitions,
    firstLocationIdx);

#ifdef ENABLE_DEBUG
  int trueIdx = locations[localLocIdx].getUniqueId();
  if (visitMsg.locationIdx != trueIdx) {
    CkAbort("Error on chare %d: Visit by person %d to loc %d recieved by "
        "loc %d (local %d)\n",
        thisIndex, visitMsg.personIdx, visitMsg.locationIdx, trueIdx,
        localLocIdx);
  }
#endif

  // CkPrintf("    Chare %d: Person %d visiting loc %d from %d to %d\n",
  //   thisIndex, visitMsg.personIdx, visitMsg.locationIdx,
  //   //localLocIdx, numLocalLocations,
  //   visitMsg.visitStart, visitMsg.visitEnd);

  // Wrap visit info...
  Event arrival { ARRIVAL, visitMsg.personIdx, visitMsg.personState,
    visitMsg.visitStart };
  Event departure { DEPARTURE, visitMsg.personIdx, visitMsg.personState,
    visitMsg.visitEnd };
  Event::pair(&arrival, &departure);

  // ...and queue it up at the appropriate location
  locations[localLocIdx].addEvent(arrival);
  locations[localLocIdx].addEvent(departure);
}

void Locations::ComputeInteractions() {
  int firstLocalIndex = getFirstIndex(thisIndex, numLocations,
    numLocationPartitions, firstLocationIdx);

  // traverses list of locations
  Counter numVisits = 0;
  Counter numInteractions = 0;
  for (Location &loc : locations) {
    Counter locVisits = loc.events.size() / 2;
    numVisits += locVisits;

    Counter locInters = loc.processEvents(diseaseModel, contactModel);
    numInteractions += locInters;

    // if (0 < locInters) {
    //   CkPrintf("    Chare %d: loc %d found %d interactions from %d visits\n",
    //       thisIndex, loc.getUniqueId(), locInters, locVisits);
    // }
  }
#if ENABLE_DEBUG >= DEBUG_VERBOSE
  CkCallback cb(CkReductionTarget(Main, ReceiveInteractionsCount), mainProxy);
  contribute(sizeof(Counter), &numInteractions,
      CONCAT(CkReduction::sum_, COUNTER_REDUCTION_TYPE), cb);
#endif

#if ENABLE_DEBUG >= DEBUG_PER_CHARE
  if (0 == day) {
    CkPrintf("    Process %d, thread %d: "COUNTER_PRINT_TYPE" visits, "
        COUNTER_PRINT_TYPE" interactions, %lu locations\n",
        CkMyNode(), CkMyPe(), numVisits, numInteractions, locations.size());
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
#endif  // ENABLE_LB

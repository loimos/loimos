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
#include "Location.h"
#include "Person.h"
#include "Event.h"
#include "Extern.h"
#include "Defs.h"
#include "contact_model/ContactModel.h"
#include "readers/Preprocess.h"
#include "readers/DataReader.h"
#include "intervention_model/Intervention.h"
#include "pup_stl.h"

#include <algorithm>
#include <queue>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>

std::uniform_real_distribution<> Locations::unitDistrib(0.0, 1.0);

Locations::Locations(int seed, std::string scenarioPath) {
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
  generator.seed(seed + thisIndex);

  // Init contact model
  contactModel = createContactModel();
  contactModel->setGenerator(&generator);

  int numInterventions = diseaseModel->getNumLocationInterventions();
  locations.reserve(numLocalLocations);
  for (int i = 0; i < numLocalLocations; i++) {
    locations.emplace_back(diseaseModel->locationAttributes,
      numInterventions, firstLocalLocationIdx + i);
  }

  // Load application data
  if (!syntheticRun) {
    loadLocationData(scenarioPath);
  }

  for (Location &l : locations) {
    for (int i = 0; i < numInterventions; ++i) {
      const Intervention<Location> &inter = diseaseModel->getLocationIntervention(i);
      l.toggleCompliance(i, inter.willComply(l, &generator));
    }
  }

#if ENABLE_DEBUG == DEBUG_PER_INTERACTION
  interactionsFile = new std::ofstream(scenarioPath + "interactions_chare_"
      + std::to_string(thisIndex) + ".csv");
#else
  interactionsFile = NULL;
#endif

  // Notify Main
  #ifdef USE_HYPERCOMM
  contribute(CkCallback(CkReductionTarget(Main, CharesCreated), mainProxy));
  #endif
}

Locations::Locations(CkMigrateMessage *msg) {}

void Locations::loadLocationData(std::string scenarioPath) {
  double startTime = CkWallTimer();

  // Load in location information.
  Id startingLineIndex = getGlobalIndex(0, thisIndex, numLocations,
    numLocationPartitions, firstLocationIdx) - firstLocationIdx;
  Id endingLineIndex = startingLineIndex + numLocalLocations;
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
  locationCache.seekg(thisIndex * sizeof(CacheOffset));
  CacheOffset locationOffset;
  locationCache.read(reinterpret_cast<char *>(&locationOffset), sizeof(CacheOffset));
  locationData.seekg(locationOffset);

  // Read in our location data.
  readData(&locationData, diseaseModel->locationDef,
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
  }
}

void Locations::ReceiveVisitMessages(VisitMessage visitMsg) {
  // adding person to location visit list
  Id localLocIdx = getLocalIndex(visitMsg.locationIdx, thisIndex,
    numLocations, numLocationPartitions, firstLocationIdx);

  // Interventions might cause us to reject some visits
  if (!locations[localLocIdx].acceptsVisit(visitMsg)) {
    return;
  }

#ifdef ENABLE_DEBUG
  Id trueIdx = locations[localLocIdx].getUniqueId();
  if (visitMsg.locationIdx != trueIdx) {
    CkPrintf("Error on chare %d: Visit by person "ID_PRINT_TYPE
        " to loc "ID_PRINT_TYPE" recieved by "
        "loc "ID_PRINT_TYPE" (local "ID_PRINT_TYPE")\n",
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
    visitMsg.transmissionModifier, visitMsg.visitStart };
  Event departure { DEPARTURE, visitMsg.personIdx, visitMsg.personState,
    visitMsg.transmissionModifier, visitMsg.visitEnd };
  Event::pair(&arrival, &departure);

  // ...and queue it up at the appropriate location
  locations[localLocIdx].addEvent(arrival);
  locations[localLocIdx].addEvent(departure);
}

void Locations::ComputeInteractions() {
  Id firstLocalIndex = getFirstIndex(thisIndex, numLocations,
    numLocationPartitions, firstLocationIdx);

  // traverses list of locations
  Counter numVisits = 0;
  Counter numInteractions = 0;
  for (Location &loc : locations) {
    Counter locVisits = loc.events.size() / 2;
    numVisits += locVisits;

    Counter locInters = processEvents(&loc);
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

Counter Locations::processEvents(Location *loc) {
  std::vector<Event> *arrivals;
  #if ENABLE_DEBUG >= DEBUG_VERBOSE
  Counter numInteractions = 0;
  Counter numPresent = 0;
  Counter numVisits = loc->events.size() / 2;
  double startTime = CkWallTimer();
  #endif

  std::sort(loc->events.begin(), loc->events.end());
  for (const Event &event : loc->events) {
    #if ENABLE_DEBUG >= DEBUG_VERBOSE
    if (ARRIVAL == event.type) {
      numPresent++;
    } else {
      numPresent--;
      numInteractions += numPresent;
      saveInteractions(*loc, event, interactionsFile);
    }
    #endif

    if (diseaseModel->isSusceptible(event.personState)) {
      arrivals = &susceptibleArrivals;

    } else if (diseaseModel->isInfectious(event.personState)) {
      arrivals = &infectiousArrivals;

    // If a person can niether infect other people nor be infected themself,
    // we can just ignore their comings and goings
    } else {
      continue;
    }

    if (ARRIVAL == event.type) {
      arrivals->push_back(event);
      std::push_heap(arrivals->begin(), arrivals->end(), Event::greaterPartner);

    } else if (DEPARTURE == event.type) {
      // Remove the arrival event corresponding to this departure
      std::pop_heap(arrivals->begin(), arrivals->end(), Event::greaterPartner);
      arrivals->pop_back();

      onDeparture(loc, event);
    }
  }
  loc->events.clear();
  interactions.clear();

  #if ENABLE_DEBUG >= DEBUG_VERBOSE
  double p = contactModel->getContactProbability(*loc);
  Counter total = static_cast<Counter>(p * numInteractions);

  #if ENABLE_DEBUG == DEBUG_LOCATION_SUMMARY
  if (0 != numInteractions) {
    double elapsedTime = CkWallTimer() - startTime;
    CkPrintf("      %d,%d,%f,"COUNTER_PRINT_TYPE","COUNTER_PRINT_TYPE","
        COUNTER_PRINT_TYPE",%f\n",
        loc->getUniqueId(), loc->getValue(maxSimVisitsIdx).int_b10,
        p, numInteractions, total, numVisits, elapsedTime);
  }
  #endif  // DEBUG_LOCATION_SUMMARY
  return total;
  #else
  return 0;
  #endif  // DEBUG_VERBOSE
}

#if ENABLE_DEBUG >= DEBUG_VERBOSE
Counter Locations::saveInteractions(const Location &loc,
    const Event &departure, std::ofstream *out) const {
  Counter count = 0;
  for (const Event &a : susceptibleArrivals) {
    if (NULL != out) {
      *out << loc.getUniqueId() << "," << departure.personIdx << ","
      << departure.partnerTime << ","  << departure.scheduledTime << ","
      << a.personIdx << "," << a.scheduledTime << "," << a.partnerTime
      << std::endl;
    }
    if (Event::overlap(a, departure)) {
      count++;
    }
  }
  for (const Event &a : infectiousArrivals) {
    if (NULL != out) {
      *out << loc.getUniqueId() << "," << departure.personIdx << ","
      << departure.partnerTime << ","  << departure.scheduledTime << ","
      << a.personIdx << "," << a.scheduledTime << "," << a.partnerTime
      << std::endl;
    }
    if (Event::overlap(a, departure)) {
      count++;
    }
  }
  return count;
}
#endif  // DEBUG_VERBOSE

// Simple dispatch to the susceptible/infectious depature handlers
inline void Locations::onDeparture(Location *loc, const Event& departure) {
  if (diseaseModel->isSusceptible(departure.personState)) {
    onSusceptibleDeparture(loc, departure);

  } else if (diseaseModel->isInfectious(departure.personState)) {
    onInfectiousDeparture(loc, departure);
  }
}

void Locations::onSusceptibleDeparture(Location *loc,
    const Event& susceptibleDeparture) {
  // Each infectious person at this location might have infected this
  // susceptible person
  for (const Event &infectiousArrival : infectiousArrivals) {
    registerInteraction(loc, susceptibleDeparture, infectiousArrival,
      // The start time is whichever arrival happened later
      std::max(infectiousArrival.scheduledTime,
        susceptibleDeparture.partnerTime),
      susceptibleDeparture.scheduledTime);
  }

  sendInteractions(loc, susceptibleDeparture.personIdx);
}

void Locations::onInfectiousDeparture(Location *loc,
    const Event& infectiousDeparture) {
  // Each susceptible person at this location might have been infected by this
  // infectious person
  for (const Event &susceptibleArrival : susceptibleArrivals) {
    registerInteraction(loc, susceptibleArrival, infectiousDeparture,
      // The start time is whichever arrival happened later
      std::max(susceptibleArrival.scheduledTime,
        infectiousDeparture.partnerTime),
      infectiousDeparture.scheduledTime);
  }
}

inline void Locations::registerInteraction(Location *loc,
    const Event &susceptibleEvent, const Event &infectiousEvent,
    Time startTime, Time endTime) {
  if (!contactModel->madeContact(susceptibleEvent, infectiousEvent, *loc)) {
    return;
  }

  double propensity = diseaseModel->getPropensity(susceptibleEvent.personState,
    infectiousEvent.personState, startTime, endTime,
    susceptibleEvent.transmissionModifier, infectiousEvent.transmissionModifier);

  // Note that this will create a new vector if this is the first potential
  // infection for the susceptible person in question
  Interaction inter { propensity, infectiousEvent.personIdx,
    infectiousEvent.personState, startTime, endTime };
  interactions[susceptibleEvent.personIdx].emplace_back(inter);
}

// Simple helper function which send the list of interactions with the
// specified person to the appropriate People chare
inline void Locations::sendInteractions(Location *loc,
    Id personIdx) {
  PartitionId peoplePartitionIdx = getPartitionIndex(personIdx,
    numPeople, numPeoplePartitions, firstPersonIdx);

  InteractionMessage interMsg(loc->getUniqueId(), personIdx,
      interactions[personIdx]);
  #ifdef USE_HYPERCOMM
  Aggregator* agg = aggregatorProxy.ckLocalBranch();
  if (agg->interact_aggregator) {
    agg->interact_aggregator->send(peopleArray[peoplePartitionIdx], interMsg);
  } else {
  #endif  // USE_HYPERCOMM
    peopleArray[peoplePartitionIdx].ReceiveInteractions(interMsg);
  #ifdef USE_HYPERCOMM
  }
  #endif  // USE_HYPERCOMM

  // CkPrintf(
  //   "    Sending %d interactions to person %d in partition %d\r\n",
  //   (int) interactions[personIdx].size(),
  //   personIdx,
  //   peoplePartitionIdx
  // );

  // Free up space where we were storing interactions data. This also prevents
  // interactions from being sent multiple times if this person has multiple
  // visits to this location
  interactions.erase(personIdx);
}

void Locations::ReceiveIntervention(PartitionId interventionIdx) {
  for (Location &location : locations) {
    const Intervention<Location> &inter =
      diseaseModel->getLocationIntervention(interventionIdx);
    if (location.willComply(interventionIdx)
        && inter.test(location, &generator)) {
      inter.apply(&location);
    }
  }
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

/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "Types.h"
#include "Locations.h"
#include "Event.h"
#include "Scenario.h"
#include "DiseaseModel.h"
#include "Location.h"
#include "Person.h"
#include "Extern.h"
#include "Defs.h"
#include "Partitioner.h"
#include "DiscreteEventSimulation.h"
#include "contact_model/ContactModel.h"
#include "readers/Preprocess.h"
#include "readers/DataReader.h"
#include "intervention_model/InterventionModel.h"
#include "intervention_model/Intervention.h"
#include "pup_stl.h"

#include <algorithm>
#include <queue>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_set>

std::uniform_real_distribution<> Locations::unitDistrib(0.0, 1.0);

Locations::Locations(int seed, std::string scenarioPath) {
  scenario = globScenario.ckLocalBranch();
  day = 0;

  // Must be set to true to make AtSync work
  usesAtSync = true;

  // Getting number of locations assigned to this chare
  Partitioner *partitioner = scenario->partitioner;
  numLocalLocations = partitioner->getLocationPartitionSize(thisIndex);
  firstLocalLocationIdx = partitioner->getGlobalLocationIndex(0, thisIndex);

#ifdef ENABLE_DEBUG
  Id firstLocationIdx = partitioner->getGlobalLocationIndex(0, 0);
  Id lastLocationIdx = firstLocationIdx + scenario->numLocations;
  if (outOfBounds(firstLocationIdx, lastLocationIdx, firstLocalLocationIdx)) {
    CkAbort("Error on chare %d: first location index ("
      ID_PRINT_TYPE") out of bounds [" ID_PRINT_TYPE ", " ID_PRINT_TYPE ")",
      thisIndex, firstLocalLocationIdx, firstLocationIdx, lastLocationIdx);
  }
#endif
#if ENABLE_DEBUG >= DEBUG_PER_CHARE
  CkPrintf("  Chare %d has %d locs (%d-%d)\n",
      thisIndex, numLocalLocations, firstLocalLocationIdx,
      firstLocalLocationIdx + numLocalLocations - 1);
#endif

  InterventionModel *interventions = scenario->interventionModel;
  int numInterventions = interventions->getNumLocationInterventions();
  locations.reserve(numLocalLocations);
  for (int i = 0; i < numLocalLocations; i++) {
    // Seed random number generator via branch ID for reproducibility
    locations.emplace_back(scenario->locationAttributes,
      numInterventions, firstLocalLocationIdx + i,
      scenario->numDaysWithDistinctVisits);
  }

  // Load application data if not running on the fly.
  // If the scenario was on the fly, this will be done in ReceiveVisitSchedule,
  // invoked from the people chares.
  if (!scenario->isOnTheFly()) {
    loadLocationData(scenarioPath);
  }

  for (Location &l : locations) {
    l.setSeed(seed);
    for (int i = 0; i < numInterventions; ++i) {
      const Intervention<Location> &inter = interventions->getLocationIntervention(i);
      l.toggleCompliance(i, inter.willComply(l, l.getGenerator()));
    }
  }

#if OUTPUT_FLAGS & OUTPUT_OVERLAPS
  interactionsFile = new std::ofstream(scenario->outputPath + "interactions_chare_"
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

  std::string scenarioId = scenario->scenarioId;
  std::ifstream locationData(scenario->scenarioPath + "locations.csv");
  std::ifstream locationCache(scenario->scenarioPath + scenarioId
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
  readData(&locationData, scenario->locationDef,
      &locations, scenario->numLocations);
  locationData.close();
  locationCache.close();

  // Let contact model add any attributes it needs to the locations
  ContactModel *contactModel = scenario->contactModel;
  for (Location &location : locations) {
    contactModel->computeLocationValues(&location);
  }

  // Open activity data and cache.
  std::ifstream visitData(scenarioPath + "visits.csv");
  std::ifstream activityCache(scenarioPath + scenarioId
      + "_visits.cache", std::ios_base::binary);
  if (!visitData || !activityCache) {
    CkAbort("Could not open activity input.");
  }

  // Load preprocessing meta data.
  CacheOffset *buf = reinterpret_cast<CacheOffset *>(
    malloc(sizeof(CacheOffset) * scenario->numDaysWithDistinctVisits));
  for (Id c = 0; c < numLocalLocations; c++) {
    std::vector<CacheOffset> *data_pos = &locations[c].visitOffsetByDay;
    Id currId = locations[c].getUniqueId();

    // Read in their activity data offsets.
    activityCache.seekg(sizeof(CacheOffset) * scenario->numDaysWithDistinctVisits
       * scenario->partitioner->getLocationCacheIndex(currId));
    activityCache.read(reinterpret_cast<char *>(buf),
      sizeof(CacheOffset) * scenario->numDaysWithDistinctVisits);
    for (int day = 0; day < scenario->numDaysWithDistinctVisits; day++) {
      data_pos->push_back(buf[day]);
    }
  }
  free(buf);

  loadVisitData(&visitData);

  visitData.close();

#if ENABLE_DEBUG >= DEBUG_PER_CHARE
  CkPrintf("  Chare %d took %f s to load locations\n", thisIndex,
      CkWallTimer() - startTime);
#endif
}

void Locations::loadVisitData(std::ifstream *visitData) {
  loimos::proto::CSVDefinition *visitDef = scenario->visitDef;
  Time firstDay = 0;
  if (visitDef->has_start_time()) {
    firstDay = visitDef->start_time().days();
  }

  #ifdef ENABLE_DEBUG
    Id numVisits = 0;
  #endif
  Id numDaysWithDistinctVisits = scenario->numDaysWithDistinctVisits;
  for (Location &location : locations) {
    location.visitsByDay.reserve(numDaysWithDistinctVisits);
    for (int day = 0; day < numDaysWithDistinctVisits; ++day) {
      Time nextDaySecs = getSeconds(day + 1, firstDay);

      // Seek to correct position in file.
      CacheOffset seekPos = location
        .visitOffsetByDay[day % numDaysWithDistinctVisits];
      if (seekPos == EMPTY_VISIT_SCHEDULE) {
#if ENABLE_DEBUG >= DEBUG_PER_CHARE
        CkPrintf("  Chare %d: Location %d has no visits on day %d\n",
            thisIndex, location.getUniqueId(), day);
#endif
        continue;
      }

      visitData->seekg(seekPos, std::ios_base::beg);

      // Start reading
      Id locationId = -1;
      Id personId = -1;
      Time visitStart = -1;
      Time visitDuration = -1;
      Time visitEnd = -1;
      std::tie(locationId, personId, visitStart, visitDuration) =
        parseActivityStream(visitData, scenario->visitDef, NULL);

#if ENABLE_DEBUG >= DEBUG_PER_OBJECT
      if (0 == locationId % 10000) {
        CkPrintf("  locations chare %d, location %d reading from %u on day %d\n",
            thisIndex, location.getUniqueId(), seekPos, day);
        CkPrintf("  Location %d (%d) on day %d first visit: %d to %d, "
            "at loc %d\n", location.getUniqueId(), locationId, day,
            visitStart, visitStart + visitDuration, locationId);
      }
#endif

      // Seek while same location on same day
      while (locationId == location.getUniqueId() && visitStart < nextDaySecs) {
        // Save visit info
        visitEnd = visitStart + visitDuration;
        while (visitEnd > nextDaySecs) {
          int endDay = getDay(visitEnd, firstDay) % numDaysWithDistinctVisits;
          Time newStart = getSeconds(endDay, firstDay);
          location.visitsByDay[endDay].emplace_back(locationId, personId, -1,
              newStart, visitEnd, 1.0);
          visitEnd = std::max(nextDaySecs, visitEnd - DAY_LENGTH);
        }

        location.visitsByDay[day].emplace_back(locationId, personId, -1,
            visitStart, visitEnd, 1.0);
        #ifdef ENABLE_DEBUG
          numVisits++;
        #endif

        std::tie(locationId, personId, visitStart, visitDuration) =
          parseActivityStream(visitData,
              scenario->visitDef, NULL);
      }

      // CkPrintf("  Chare %d: location %d has %u visits on day %d (offset %u)\n",
      //     thisIndex, location.getUniqueId(), location.visitsByDay[day].size(),
      //     day, seekPos);
    }
  }
  #if ENABLE_DEBUG >= DEBUG_VERBOSE
    CkCallback cb(CkReductionTarget(Main, ReceiveVisitsLoadedCount), mainProxy);
    contribute(sizeof(Id), &numVisits, CkReduction::CONCAT(sum_, ID_REDUCTION_TYPE),
      cb);
  #endif
}

void Locations::pup(PUP::er &p) {
  p | numLocalLocations;
  p | locations;
  p | day;

  if (p.isUnpacking()) {
    scenario = globScenario.ckLocalBranch();
  }
}

void Locations::ReceiveVisitSchedule(VisitScheduleMessage msg) {
  Partitioner *partitioner = scenario->partitioner;
  for (size_t d = 0; d < msg.visitsByDay.size(); d++) {
    for (const VisitMessage &visit : msg.visitsByDay[d]) {
      Id localLocIdx = partitioner->getLocalLocationIndex(visit.locationIdx, thisIndex);
      locations[localLocIdx].visitsByDay[d].push_back(visit);
    }
  }

  msg.visitsByDay.clear();
}

void Locations::SendExpectedVisitors() {
  Partitioner *partitioner = scenario->partitioner;
  std::unordered_map<PartitionId, ExpectedVisitorsMessage > visitorsFromPartition;
  for (const Location &location : locations) {
    for (const std::vector<VisitMessage> &visits : location.visitsByDay) {
      for (const VisitMessage &visit : visits) {
        PartitionId personPartition = partitioner->getPersonPartitionIndex(
          visit.personIdx);
        if (visitorsFromPartition.find(personPartition)
            == visitorsFromPartition.end()) {
          visitorsFromPartition[personPartition].destPartition = thisIndex;
        }
        visitorsFromPartition[personPartition].visitors.insert(visit.personIdx);
      }
    }
  }

  for (auto &entry : visitorsFromPartition) {
    peopleArray[entry.first].ReceiveExpectedVisitors(entry.second);
  }
  visitorsFromPartition.clear();
}

void Locations::ReceiveVisitorStates(PersonStatesMessage msg) {
  for (size_t i = 0; i < msg.states.size(); i++) {
    PersonState *state = &msg.states[i];
    std::memcpy(&visitorStates[state->uniqueId], state, sizeof(PersonState));
  }

  msg.states.clear();
}


void Locations::QueueVisits() {
  queueVisitsImpl(locations, scenario, day, visitorStates);
  ComputeInteractions();
}

void Locations::ComputeInteractions() {
  Counter numVisits = 0;
  Counter numInteractions = 0;
  for (Location &loc : locations) {
    Counter locVisits = loc.events.size() / 2;
    numVisits += locVisits;

    Counter locInters = processEvents(&loc, scenario, interactionsFile, thisIndex);
    numInteractions += locInters;
  }
#if ENABLE_DEBUG >= DEBUG_VERBOSE
  CkCallback cb(CkReductionTarget(Main, ReceiveInteractionsCount), mainProxy);
  contribute(sizeof(Counter), &numInteractions,
      CONCAT(CkReduction::sum_, COUNTER_REDUCTION_TYPE), cb);
#endif

#if ENABLE_DEBUG >= DEBUG_PER_CHARE
  if (0 == day) {
    CkPrintf("    Process %d, thread %d: " COUNTER_PRINT_TYPE " visits, "
        COUNTER_PRINT_TYPE" interactions, %lu locations\n",
        CkMyNode(), CkMyPe(), numVisits, numInteractions, locations.size());
  }
#endif
  day++;
}

void Locations::ComputeInteractions() {
  Counter numVisits = 0;
  Counter numInteractions = 0;
  for (Location &loc : locations) {
    Counter locVisits = loc.events.size() / 2;
    numVisits += locVisits;

    // Map that will be populated with person ID to their list of interactions
    std::unordered_map<Id, std::vector<Interaction>> interactions;
    Counter locInters = processEvents(&loc, scenario, interactionsFile, thisIndex,
      &interactions);
    numInteractions += locInters;

    sendInteractions(&loc, &interactions);
  }
#if ENABLE_DEBUG >= DEBUG_VERBOSE
  CkCallback cb(CkReductionTarget(Main, ReceiveInteractionsCount), mainProxy);
  contribute(sizeof(Counter), &numInteractions,
      CONCAT(CkReduction::sum_, COUNTER_REDUCTION_TYPE), cb);
#endif

#if ENABLE_DEBUG >= DEBUG_PER_CHARE
  if (0 == day) {
    CkPrintf("    Process %d, thread %d: " COUNTER_PRINT_TYPE " visits, "
        COUNTER_PRINT_TYPE" interactions, %lu locations\n",
        CkMyNode(), CkMyPe(), numVisits, numInteractions, locations.size());
  }
#endif
  day++;
}

// Sends all person interactions. Groups interactions by person partition and sends each
// partition's interaction messages as a single batch.
void Locations::sendInteractions(Location *loc, std::unordered_map<Id,
  std::vector<Interaction>> *interactions) {
  for (auto &[personPartition, messages] :
    createPartitionToMessagesMapping(loc, interactions)) {
    peopleArray[personPartition].ReceiveInteractions(messages);
  }
}

// Groups interaction messages by destination person partition and returns this mapping.
// Creates one InteractionMessage per person and stores it in the bucket corresponding
// to that person's partition.
std::unordered_map<PartitionId, std::vector<InteractionMessage>>
Locations::createPartitionToMessagesMapping(Location *loc, std::unordered_map<Id,
  std::vector<Interaction>> *interactions) {
  Partitioner *partitioner = scenario->partitioner;
  std::unordered_map<PartitionId, std::vector<InteractionMessage>> ans;
  for (const auto &[personIdx, interactionsList] : *interactions) {
    PartitionId personPartition = partitioner->getPersonPartitionIndex(personIdx);

    #ifdef ENABLE_DEBUG
      if (outOfBounds(0, partitioner->getNumPersonPartitions(), personPartition)) {
        CkAbort("Error on chare %d: sending exposures at "
          ID_PRINT_TYPE" to person " ID_PRINT_TYPE " on chare "
          PARTITION_ID_PRINT_TYPE" outside of valid range [0, "
          PARTITION_ID_PRINT_TYPE")\n", thisIndex, loc->getUniqueId(),
          personIdx, personPartition, partitioner->getNumPersonPartitions());
      }
    #endif

    // Create message for this person
    InteractionMessage interMsg(loc->getUniqueId(), personIdx, interactionsList);

    // Add to the correct partition bucket
    ans[personPartition].emplace_back(interMsg);
  }
  return ans;
}

void Locations::ReceiveVisitMessages(VisitMessage visitMsg) {
  // adding person to location visit list
  Id localLocIdx = scenario->partitioner->getLocalLocationIndex(
    visitMsg.locationIdx, thisIndex);

#ifdef ENABLE_DEBUG
  if (outOfBounds(0l, numLocalLocations, localLocIdx)) {
    CkAbort("Error on chare %d: recieved visit to location ("
      ID_PRINT_TYPE" loc, " ID_PRINT_TYPE " glob) outside of valid range [0, "
      ID_PRINT_TYPE") loc\n", thisIndex, localLocIdx, visitMsg.locationIdx,
      numLocalLocations);
  }
  Id trueIdx = locations[localLocIdx].getUniqueId();
  if (visitMsg.locationIdx != trueIdx) {
    CkAbort("Error on chare %d: Visit by person " ID_PRINT_TYPE
        " to loc " ID_PRINT_TYPE " received by "
        "loc " ID_PRINT_TYPE " (local " ID_PRINT_TYPE ")\n",
        thisIndex, visitMsg.personIdx, visitMsg.locationIdx, trueIdx,
        localLocIdx);
  }
#endif

  // Interventions might cause us to reject some visits
  if (!visitMsg.isActive()) {
    return;
  }

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

#ifdef ENABLE_DEBUG
  if (arrival.scheduledTime > departure.scheduledTime) {
    CkAbort("Error on chare %d: visit by " ID_PRINT_TYPE " to loc " ID_PRINT_TYPE "\n"
      "has departure (%d) before arrival (%d)\n",
      thisIndex, visitMsg.personIdx, trueIdx, arrival.scheduledTime,
      departure.scheduledTime);
  }
#endif

  // ...and queue it up at the appropriate location
  Location &loc = locations[localLocIdx];
#ifdef ENABLE_SC
  bool isInfectious = scenario->diseaseModel->isInfectious(visitMsg.personState);
  if (!loc.anyInfectious && isInfectious) {
    loc.anyInfectious = true;
  }
#endif

  loc.addEvent(arrival);
  loc.addEvent(departure);
}

void Locations::ReceiveIntervention(PartitionId interventionIdx) {
  InterventionModel *interventions = scenario->interventionModel;
  std::unordered_set<Id> applied;
  std::unordered_set<Id> removed;
  interventions->applyIntervention(interventionIdx, &locations, &applied, &removed);
}

void Locations::ReceiveVisitIntervention(VisitInterventionMessage msg) {
  InterventionModel *interventions = scenario->interventionModel;
  const Intervention<Person> &inter =
    interventions->getPersonIntervention(msg.interventionIdx);
  inter.apply(&locations, msg.affectedPeople, msg.previouslyAffectedPeople);
}

#ifdef ENABLE_LB
void Locations::ResumeFromSync() {
#if ENABLE_DEBUG >= DEBUG_PER_CHARE
  CkPrintf("  Done load balancing on location chare %d\n", thisIndex);
#endif

  CkCallback cb(CkReductionTarget(Main, locationsLBComplete), mainProxy);
  contribute(cb);
}
#endif  // ENABLE_LB

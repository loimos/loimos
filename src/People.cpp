/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "Types.h"
#include "People.h"
#include "Defs.h"
#include "Extern.h"
#include "Interaction.h"
#include "DiseaseModel.h"
#include "Person.h"
#include "readers/Preprocess.h"
#include "readers/DataReader.h"
#include "intervention_model/Intervention.h"

#ifdef USE_HYPERCOMM
  #include "Aggregator.h"
#endif  // USE_HYPERCOMM

#include <tuple>
#include <limits>
#include <queue>
#include <cmath>
#include <string>
#include <iostream>
#include <fstream>
#include <functional>
#include <algorithm>
#include <memory>

std::uniform_real_distribution<> unitDistrib(0, 1);
#define ONE_ATTR 1
#define DEFAULT_

People::People(int seed, std::string scenarioPath) {
  // Must be set to true to make AtSync work
  usesAtSync = true;

  day = 0;
  diseaseModel = globDiseaseModel.ckLocalBranch();

  // Allocate space to summarize the state summaries for every day
  DiseaseState totalStates = diseaseModel->getNumberOfStates();
  stateSummaries.resize(totalStates * numDays, 0);

  // Get the number of people assigned to this chare
  numLocalPeople = diseaseModel->getPersonPartitionSize(thisIndex);
  Id firstLocalPersonIdx = diseaseModel->getGlobalPersonIndex(0, thisIndex);
#if ENABLE_DEBUG >= DEBUG_PER_CHARE
  CkPrintf("  Chare %d has %d people (%d-%d)\n",
      thisIndex, numLocalPeople, firstLocalPersonIdx,
      firstLocalPersonIdx + numLocalPeople - 1);

  double startTime = CkWallTimer();
#endif
#ifdef ENABLE_DEBUG
  Id firstPersonIdx = diseaseModel->getGlobalPersonIndex(0, 0);
  Id lastPersonIdx = numPeople + firstPersonIdx;
  if (outOfBounds(firstPersonIdx, lastPersonIdx,
      firstLocalPersonIdx)) {
    CkAbort("Error on chare %d: first person index ("
      ID_PRINT_TYPE ") out of bounds [" ID_PRINT_TYPE ", " ID_PRINT_TYPE ")",
      thisIndex, firstLocalPersonIdx, firstPersonIdx, lastPersonIdx);
  }
#endif

  int numInterventions = diseaseModel->getNumPersonInterventions();
  people.reserve(numLocalPeople);
  for (Id i = 0; i < numLocalPeople; i++) {
    people.emplace_back(diseaseModel->personAttributes,
        numInterventions, 0, std::numeric_limits<Time>::max(),
        numDaysWithDistinctVisits);
  }

  if (syntheticRun) {
    generatePeopleData(firstLocalPersonIdx, seed);
    generateVisitData();
  } else {
    // Load in people data from file.
    loadPeopleData(scenarioPath);
  }

for (Person &p : people) {
  if (!syntheticRun) {
    // Need to wait until after unique ids are set in case they don't start at 0
    p.setSeed(seed);
  }
  for (int i = 0; i < numInterventions; ++i) {
    const Intervention<Person> &inter = diseaseModel->getPersonIntervention(i);
    p.toggleCompliance(i, inter.willComply(p, p.getGenerator()));
  }
}

#if ENABLE_DEBUG >= DEBUG_PER_CHARE
  CkPrintf("  Chare %d took %f s to load people\n", thisIndex,
      CkWallTimer() - startTime);
#endif

#if OUTPUT_FLAGS & OUTPUT_EXPOSURES
  exposuresFile = new std::ofstream(outputPath + "exposures_chare_"
      + std::to_string(thisIndex) + ".csv");
  *exposuresFile << "tick,sus_pid,inf_pid,start_time,end_time,propensity"
      << std::endl;
#else
  exposuresFile = NULL;
#endif

#if OUTPUT_FLAGS & OUTPUT_TRANSITIONS
  transitionsFile = new std::ofstream(outputPath + "transitions_chare_"
      + std::to_string(thisIndex) + ".csv");
  *transitionsFile << "tick,pid,exit_state,contact_pid,contact_start"
      << std::endl;
#else
  transitionsFile = NULL;
#endif

  // Notify Main
  mainProxy.CharesCreated();
}

People::People(CkMigrateMessage *msg) {}

void People::generatePeopleData(Id firstLocalPersonIdx, int seed) {
  // Init peoples ids and randomly init ages.
  std::uniform_int_distribution<int> age_dist(0, 100);
  int ageIndex = diseaseModel->personAttributes.getAttributeIndex("age");
  for (Id i = 0; i < numLocalPeople; i++) {
    Person &p = people[i];
    p.setUniqueId(firstLocalPersonIdx + i);
    // Local seed depends on uniqueId
    p.setSeed(seed);

    std::vector<union Data> data = p.getData();
    if (-1 != ageIndex) {
      data[ageIndex].int32_val = age_dist(*p.getGenerator());
    }
    p.state = diseaseModel->getHealthyState(data);

    // We set persons next state to equal current state to signify
    // that they are not in a disease model progression.
    p.next_state = p.state;
  }
}

/**
 * Randomly generates an itinerary (number of visits to random locations)
 * for each person
 */
void People::generateVisitData() {
  int totalNumVisits = 0;

  // Model number of visits as a poisson distribution.
  std::poisson_distribution<int> num_visits_generator(averageDegreeOfVisit);

  // Model visit distance as poisson distribution.
  std::poisson_distribution<Id> visit_distance_generator(LOCATION_LAMBDA);

  // Model visit times as uniform.
  std::uniform_int_distribution<Time> time_dist(0, DAY_LENGTH);  // in seconds
  std::priority_queue<Time, std::vector<Time>, std::greater<Time> > times;

  // Flip a coin to decide directions in each dimension
  std::uniform_int_distribution<int> dir_gen(0, 1);

  // Calculate minigrid sizes.
  Id numLocationsPerPartition = getNumElementsPerPartition(
    numLocations, numLocationPartitions);
  Id locationPartitionWidth = synLocalLocationGridWidth;
  Id locationPartitionHeight = synLocalLocationGridHeight;
  Id locationPartitionGridWidth = synLocationPartitionGridWidth;
#if ENABLE_DEBUG >= DEBUG_BASIC
  if (0 == thisIndex) {
    CkPrintf("location grid at each chare is %d by %d\r\n",
      locationPartitionWidth, locationPartitionHeight);
  }
#endif

  // Choose one location partition for the people in this partition to call home
  Id homePartitionIdx = thisIndex % numLocationPartitions;
  Id homePartitionX = homePartitionIdx % locationPartitionGridWidth;
  Id homePartitionY = homePartitionIdx / locationPartitionGridWidth;
  Id homePartitionStartX = homePartitionX * locationPartitionWidth;
  Id homePartitionStartY = homePartitionY * locationPartitionHeight;
  Id homePartitionNumLocations = getNumLocalElements(
    numLocations, numLocationPartitions, homePartitionIdx);

  // Calculate schedule for each person.
  Id firstLocationIdx = diseaseModel->getGlobalLocationIndex(0, 0);
  for (Person &p : people) {
    Id personIdx = p.getUniqueId();

    // Calculate home location
    Id localPersonIdx = (personIdx - firstLocationIdx) % homePartitionNumLocations;
    Id homeX = homePartitionStartX + localPersonIdx % locationPartitionWidth;
    Id homeY = homePartitionStartY + localPersonIdx / locationPartitionWidth;

    p.visitsByDay.resize(numDaysWithDistinctVisits);
    for (std::vector<VisitMessage> &visits : p.visitsByDay) {
      std::default_random_engine *generator = p.getGenerator();
      // Get random number of visits for this person.
      int numVisits = num_visits_generator(*generator);
      // Randomly generate start and end times for each visit,
      // using a priority queue ensures the times are in order.
      for (int j = 0; j < 2 * numVisits; j++) {
        times.push(time_dist(*generator));
      }

      totalNumVisits += numVisits;

      // Randomly pick nearby location for person to visit.
      for (int j = 0; j < numVisits; j++) {
        // Generate visit start and end times.
        Time visitStart = times.top();
        times.pop();
        Time visitEnd = times.top();
        times.pop();
        // Skip empty visits.
        if (visitStart == visitEnd)
          continue;

        // Get number of locations away this person should visit.
        Id numHops = std::min(visit_distance_generator(*generator),
          synLocationGridWidth + synLocationGridHeight - 2);

        Id destinationOffsetX = 0;
        Id destinationOffsetY = 0;

        if (numHops != 0) {
          // Calculate maximum hops that can be taken from home location in each
          // direction. (i.e. might be constrained for home locations close to edge)
          Id maxHopsNegativeX = std::min(numHops, homeX);
          Id maxHopsPositiveX = std::min(numHops,
            synLocationGridWidth - 1 - homeX);
          Id maxHopsNegativeY = std::min(numHops, homeY);
          Id maxHopsPositiveY = std::min(numHops,
            synLocationGridHeight - 1 - homeY);

          // Choose random number of hops in the X direction.
          std::uniform_int_distribution<Id> dist_gen(-maxHopsNegativeX,
            maxHopsPositiveX);
          destinationOffsetX = dist_gen(*generator);

          // Travel the remaining hops in the Y direction
          numHops -= std::abs(destinationOffsetX);
          if (numHops != 0) {
            // Choose a random direction between positive and negative
            if (dir_gen(*generator) == 0) {
              // Offset positively in Y.
              destinationOffsetY = std::min(numHops, maxHopsPositiveY);
            } else {
              // Offset negatively in Y.
              destinationOffsetY = -std::min(numHops, maxHopsNegativeY);
            }
          }
        }

        // Finally calculate the index of the location to actually visit...
        Id destinationX = homeX + destinationOffsetX;
        Id destinationY = homeY + destinationOffsetY;

        // ...and translate it from 2D to 1D, respecting the 2D distribution
        // of the locations across partitions
        PartitionId partitionX = destinationX / locationPartitionWidth;
        PartitionId partitionY = destinationY / locationPartitionHeight;
        Id destinationIdx =
            (destinationX % locationPartitionWidth)
          + (destinationY % locationPartitionHeight) * locationPartitionWidth
          + partitionX * numLocationsPerPartition
          + partitionY * locationPartitionGridWidth * numLocationsPerPartition;

        visits.emplace_back(destinationIdx, personIdx, 0, visitStart,
            visitEnd, 0);

  #if ENABLE_DEBUG >= DEBUG_PER_OBJECT
        CkPrintf(
            "person %d will visit location (%d, %d) with offset (%d,%d)\r\n",
            personIdx, destinationX, destinationY, destinationOffsetX,
            destinationOffsetY);
        CkPrintf("(%d, %d) -> %d in partition (%d, %d)\r\n",
            destinationX, destinationY, destinationIdx, partitionX, partitionY);
  #endif
      }
    }
  }
}

/**
 * Loads real people data from file.
 */
void People::loadPeopleData(std::string scenarioPath) {
  std::string scenarioId = getScenarioId(numPeople, numPersonPartitions,
    numLocations, numLocationPartitions);
  std::ifstream peopleData(scenarioPath + "people.csv");
  std::ifstream peopleCache(scenarioPath + scenarioId + "_people.cache",
    std::ios_base::binary);
  if (!peopleData || !peopleCache) {
    CkAbort("Could not open person data input.");
  }

  // Find starting line for our data through people cache.
  peopleCache.seekg(thisIndex * sizeof(CacheOffset));
  CacheOffset peopleOffset;
  peopleCache.read(reinterpret_cast<char *>(&peopleOffset), sizeof(CacheOffset));
  peopleData.seekg(peopleOffset);

  // Read in from remote file.
  readData(&peopleData, diseaseModel->personDef, &people, numPeople);
  peopleData.close();
  peopleCache.close();

  // Open activity data and cache.
  std::ifstream activityData(scenarioPath + "visits.csv");
  std::ifstream activityCache(scenarioPath + scenarioId
      + "_visits.cache", std::ios_base::binary);
  if (!activityData || !activityCache) {
    CkAbort("Could not open activity input.");
  }

  // Load preprocessing meta data.
  CacheOffset *buf = reinterpret_cast<CacheOffset *>(
    malloc(sizeof(CacheOffset) * numDaysWithDistinctVisits));
  for (Id c = 0; c < numLocalPeople; c++) {
    std::vector<CacheOffset> *data_pos = &people[c].visitOffsetByDay;
    Id currId = people[c].getUniqueId();

    // Read in their activity data offsets.
    activityCache.seekg(sizeof(CacheOffset) * numDaysWithDistinctVisits
       * diseaseModel->getPersonCacheIndex(currId));
    activityCache.read(reinterpret_cast<char *>(buf),
      sizeof(CacheOffset) * numDaysWithDistinctVisits);
    for (int day = 0; day < numDaysWithDistinctVisits; day++) {
      data_pos->push_back(buf[day]);
    }
  }
  free(buf);

  for (Person &person : people) {
    person.state = diseaseModel->getHealthyState(person.getData());
    // TODO(jkitson): set compliance levels based on personInterventions
  }

  loadVisitData(&activityData);

  activityData.close();
}

void People::loadVisitData(std::ifstream *activityData) {
  loimos::proto::CSVDefinition *visitsDef = diseaseModel->activityDef;
  Time firstDay = 0;
  if (visitsDef->has_start_time()) {
    firstDay = visitsDef->start_time().days();
  }

  #ifdef ENABLE_DEBUG
    Id numVisits = 0;
  #endif
  for (Person &person : people) {
    person.visitsByDay.reserve(numDaysWithDistinctVisits);
    for (int day = 0; day < numDaysWithDistinctVisits; ++day) {
      Time nextDaySecs = getSeconds(day + 1, firstDay);

      // Seek to correct position in file.
      CacheOffset seekPos = person
        .visitOffsetByDay[day % numDaysWithDistinctVisits];
      if (seekPos == EMPTY_VISIT_SCHEDULE) {
#if ENABLE_DEBUG >= DEBUG_PER_CHARE
        CkPrintf("  Chare %d: Person %d has no visits on day %d\n",
            thisIndex, person.getUniqueId(), day);
#endif
        continue;
      }

      activityData->seekg(seekPos, std::ios_base::beg);

      // Start reading
      Id personId = -1;
      Id locationId = -1;
      Time visitStart = -1;
      Time visitDuration = -1;
      Time visitEnd = -1;
      std::tie(personId, locationId, visitStart, visitDuration) =
        parseActivityStream(activityData,
            diseaseModel->activityDef, NULL);

#if ENABLE_DEBUG >= DEBUG_PER_OBJECT
      if (0 == personId % 10000) {
        CkPrintf("  People chare %d, person %d reading from %u on day %d\n",
            thisIndex, person.getUniqueId(), seekPos, day);
        CkPrintf("  Person %d (%d) on day %d first visit: %d to %d, "
            "at loc %d\n", person.getUniqueId(), personId, day,
            visitStart, visitStart + visitDuration, locationId);
      }
#endif

      // Seek while same person on same day
      while (personId == person.getUniqueId() && visitStart < nextDaySecs) {
        // Save visit info
        visitEnd = visitStart + visitDuration;
        while (visitEnd > nextDaySecs) {
          int endDay = getDay(visitEnd, firstDay) % numDaysWithDistinctVisits;
          Time newStart = getSeconds(endDay, firstDay);
          person.visitsByDay[endDay].emplace_back(locationId, personId, -1,
              newStart, visitEnd, 1.0);
          visitEnd = std::max(nextDaySecs, visitEnd - DAY_LENGTH);
        }

        person.visitsByDay[day].emplace_back(locationId, personId, -1,
            visitStart, visitEnd, 1.0);
        #ifdef ENABLE_DEBUG
          numVisits++;
        #endif

        std::tie(personId, locationId, visitStart, visitDuration) =
          parseActivityStream(activityData,
              diseaseModel->activityDef, NULL);
      }

      // CkPrintf("  Chare %d: person %d has %u visits on day %d (offset %u)\n",
      //     thisIndex, person.getUniqueId(), person.visitsByDay[day].size(),
      //     day, seekPos);
    }
  }
  #if ENABLE_DEBUG >= DEBUG_VERBOSE
    CkCallback cb(CkReductionTarget(Main, ReceiveVisitsLoadedCount), mainProxy);
    contribute(sizeof(Id), &numVisits, CkReduction::CONCAT(sum_, ID_REDUCTION_TYPE),
      cb);
  #endif
}

void People::pup(PUP::er &p) {
  p | numLocalPeople;
  p | day;
  p | totalVisitsForDay;
  p | people;
  p | stateSummaries;

  if (p.isUnpacking()) {
    diseaseModel = globDiseaseModel.ckLocalBranch();
  }
}

void People::SendVisitMessages() {
  // Send activities for each person.
#if ENABLE_DEBUG >= DEBUG_VERBOSE
  Id minId = numPeople;
  Id maxId = 0;
  totalVisitsForDay = 0;
#endif
  int dayIdx = day % numDaysWithDistinctVisits;
  for (const Person &person : people) {
#if ENABLE_DEBUG >= DEBUG_PER_CHARE
    minId = std::min(minId, person.getUniqueId());
    maxId = std::max(maxId, person.getUniqueId());
#endif
    for (VisitMessage visitMessage : person.visitsByDay[dayIdx]) {
      visitMessage.personState = person.state;
      visitMessage.transmissionModifier = getTransmissionModifier(person);

      // Interventions may cancel some visits
      if (visitMessage.isActive()) {
        continue;
      }

      // Find process that owns that location
      PartitionId locationPartition = diseaseModel->getLocationPartitionIndex(
        visitMessage.locationIdx);
#ifdef ENABLE_DEBUG
      if (outOfBounds(0, numLocationPartitions, locationPartition)) {
        CkAbort("Error on chare %d: sending visit by "
          ID_PRINT_TYPE" to location " ID_PRINT_TYPE " on chare "
          PARTITION_ID_PRINT_TYPE " outside of valid range [0, "
          PARTITION_ID_PRINT_TYPE ")\n", thisIndex, person.getUniqueId(),
          visitMessage.locationIdx, locationPartition, numLocationPartitions);
      }
#if ENABLE_DEBUG >= DEBUG_VERBOSE
      totalVisitsForDay++;
#endif
#endif  // ENABLE_DEBUG

// Send off the visit message.
#ifdef USE_HYPERCOMM
      Aggregator* agg = aggregatorProxy.ckLocalBranch();
      if (agg->visit_aggregator) {
        agg->visit_aggregator->send(locationsArray[locationPartition], visitMessage);
        continue;
      }
#endif  // USE_HYPERCOMM

      locationsArray[locationPartition].ReceiveVisitMessages(visitMessage);
    }
  }

#if ENABLE_DEBUG >= DEBUG_VERBOSE
  CkCallback cb(CkReductionTarget(Main, ReceiveVisitsSentCount), mainProxy);
  contribute(sizeof(Counter), &totalVisitsForDay,
      CONCAT(CkReduction::sum_, COUNTER_REDUCTION_TYPE), cb);
#endif
#if ENABLE_DEBUG >= DEBUG_PER_CHARE
  if (0 == day) {
    CkPrintf("    Chare %d (P %d, T %d): %d visits, %lu people (in [%d, %d])\n",
        thisIndex, CkMyNode(), CkMyPe(), totalVisitsForDay, people.size(),
        minId, maxId);
  }
#endif
}

double People::getTransmissionModifier(const Person &person) {
  if (-1 != diseaseModel->susceptibilityIndex
      && diseaseModel->isSusceptible(person.state)) {
    return person.getValue(diseaseModel->susceptibilityIndex).double_val;
  } else if (-1 != diseaseModel->infectivityIndex
      && diseaseModel->isInfectious(person.state)) {
    return person.getValue(diseaseModel->infectivityIndex).double_val;
  }
  return 1.0;
}

void People::ReceiveInteractions(InteractionMessage interMsg) {
  Id localIdx = diseaseModel->getLocalPersonIndex(interMsg.personIdx, thisIndex);

#ifdef ENABLE_DEBUG
  Id trueIdx = people[localIdx].getUniqueId();
  if (interMsg.personIdx != trueIdx) {
    CkAbort("Error on chare %d: Person %d's exposure at loc %d recieved by "
        "person %d (local %d)\n",
        thisIndex, interMsg.personIdx, interMsg.locationIdx, trueIdx,
        localIdx);
  }

  if (outOfBounds(0l, numLocalPeople, localIdx)) {
    CkAbort("Error on chare %d: visit to location ("
      ID_PRINT_TYPE "/" ID_PRINT_TYPE") outside of valid range [0, "
      ID_PRINT_TYPE ")\n", thisIndex, localIdx, interMsg.personIdx,
      numLocalPeople);
  }
#endif

  // Just concatenate the interaction lists so that we can process all of the
  // interactions at the end of the day
  Person &person = people[localIdx];
  person.interactions.insert(person.interactions.end(),
    interMsg.interactions.cbegin(), interMsg.interactions.cend());
}

void People::ReceiveIntervention(int interventionIdx) {
  const Intervention<Person> &inter =
    diseaseModel->getPersonIntervention(interventionIdx);
  for (Person &person : people) {
    if (person.willComply(interventionIdx)
        && inter.test(person, person.getGenerator())) {
      inter.apply(&person);
    }
  }
}

void People::EndOfDayStateUpdate() {
  // Get ready to count today's states
  DiseaseState totalStates = diseaseModel->getNumberOfStates();
  int offset = totalStates * day;

  // Handle state transitions at the end of the day.
  Id infectiousCount = 0;
#if ENABLE_DEBUG >= DEBUG_VERBOSE
  Counter totalExposuresPerDay = 0;
#endif
  for (Person &person : people) {
#if ENABLE_DEBUG >= DEBUG_VERBOSE
    totalExposuresPerDay += person.interactions.size();
#endif
    ProcessInteractions(&person);
    UpdateDiseaseState(&person);

    DiseaseState resultantState = person.state;
    stateSummaries[resultantState + offset]++;
    if (diseaseModel->isInfectious(resultantState)) {
      infectiousCount++;
    }
  }

  // contributing to reduction
  CkCallback cb(CkReductionTarget(Main, ReceiveInfectiousCount), mainProxy);
  contribute(sizeof(Id), &infectiousCount,
      CONCAT(CkReduction::sum_, ID_REDUCTION_TYPE), cb);
#if ENABLE_DEBUG >= DEBUG_VERBOSE
  CkCallback expCb(CkReductionTarget(Main, ReceiveExposuresCount), mainProxy);
  contribute(sizeof(Counter), &totalExposuresPerDay,
      CONCAT(CkReduction::sum_, COUNTER_REDUCTION_TYPE), expCb);
#endif

  // Get ready for the next day
  day++;
}

void People::SendStats() {
  CkCallback cb(CkReductionTarget(Main, ReceiveStats), mainProxy);
  contribute(stateSummaries, CkReduction::CONCAT(sum_, ID_REDUCTION_TYPE),
    cb);
}

double propensityToProbability(double propensity) {
  return 1.0 - exp(-propensity);
}

void People::ProcessInteractions(Person *person) {
  double totalPropensity = 0.0;
  uint numInteractions = static_cast<uint>(person->interactions.size());
  for (const Interaction &inter : person->interactions) {
    totalPropensity += inter.propensity;
#if OUTPUT_FLAGS & OUTPUT_EXPOSURES
    // tick,sus_pid,inf_pid,start_time,end_time,propensity
    *exposuresFile << day << "," << person->getUniqueId() << ","
        << inter.infectiousIdx << "," << inter.startTime << ","
        << inter.endTime << "," << inter.propensity << std::endl;
#endif
  }

  // Detemine whether or not this person was infected...
  std::default_random_engine *generator = person->getGenerator();
  double roll = -log(unitDistrib(*generator)) / totalPropensity;

  if (roll <= 1) {
    // ...if they were, determine which interaction was responsible, by
    // choosing an interaction, with a weight equal to the propensity
    roll = std::uniform_real_distribution<>(0, totalPropensity)(*generator);
    double partialSum = 0.0;
    int interactionIdx;
    for (interactionIdx = 0; interactionIdx < numInteractions;
        ++interactionIdx) {
      partialSum += person->interactions[interactionIdx].propensity;
      if (partialSum > roll) {
        break;
      }
    }

    // TODO(jkitson): Save any useful information about the interaction which
    // caused the infection

    // Mark that exposed healthy individuals should make transition at the end
    // of the day.
    if (diseaseModel->isSusceptible(person->state)) {
      person->secondsLeftInState = -1;
      std::tie(person->next_state, std::ignore) =
        diseaseModel->transitionFromState(person->state, generator);

#if OUTPUT_FLAGS & OUTPUT_TRANSITIONS
      // tick,pid,exit_state,contact_pid,contact_start
      const Interaction &inter = person->interactions[interactionIdx];
      *transitionsFile << day << "," << person->getUniqueId() << ","
          << diseaseModel->getStateLabel(person->next_state) << ","
          << inter.infectiousIdx << "," << inter.startTime << std::endl;
#endif
    }
  }
  person->interactions.clear();
}

void People::UpdateDiseaseState(Person *person) {
  // Transition to next state or mark the passage of time
  person->secondsLeftInState -= DAY_LENGTH;
  std::default_random_engine *generator = person->getGenerator();
  if (person->secondsLeftInState <= 0) {
#if OUTPUT_FLAGS & OUTPUT_TRANSITIONS
    // State transition information for initial infections is reported when
    // they're infected
    if (!diseaseModel->isSusceptible(person->state)) {
      // tick,pid,exit_state,contact_pid,contact_start
      *transitionsFile << day << "," << person->getUniqueId() << ","
          << diseaseModel->getStateLabel(person->next_state)
          << ",-1,-1" << std::endl;
    }
#endif

    person->state = person->next_state;
    std::tie(person->next_state, person->secondsLeftInState) =
      diseaseModel->transitionFromState(person->state, generator);
  }
}

#ifdef ENABLE_LB
void People::ResumeFromSync() {
  CkCallback cb(CkReductionTarget(Main, peopleLBComplete), mainProxy);
  contribute(cb);
}
#endif  // ENABLE_LB

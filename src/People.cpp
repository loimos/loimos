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
  generator.seed(seed + thisIndex);

  // Initialize disease model
  diseaseModel = globDiseaseModel.ckLocalBranch();

  // Allocate space to summarize the state summaries for every day
  int totalStates = diseaseModel->getNumberOfStates();
  stateSummaries.resize((totalStates + 2) * numDays, 0);

  // Get the number of people assigned to this chare
  numLocalPeople = getNumLocalElements(numPeople,
      numPeoplePartitions, thisIndex);
  int firstLocalPersonIdx = getFirstIndex(thisIndex, numPeople,
      numPeoplePartitions, firstPersonIdx);
#if ENABLE_DEBUG >= DEBUG_PER_CHARE
  CkPrintf("  Chare %d has %d people (%d-%d)\n",
      thisIndex, numLocalPeople, firstLocalPersonIdx,
      firstLocalPersonIdx + numLocalPeople - 1);

  double startTime = CkWallTimer();
#endif

  int numInterventions = diseaseModel->getNumPersonInterventions();
  people.reserve(numLocalPeople);
  for (int i = 0; i < numLocalPeople; i++) {
    people.emplace_back(diseaseModel->personAttributes,
        numInterventions, 0, std::numeric_limits<Time>::max(),
        numDaysWithDistinctVisits);
  }

  if (syntheticRun) {
    generatePeopleData();
    generateVisitData();
  } else {
    // Load in people data from file.
    loadPeopleData(scenarioPath);
  }

for (Person &p : people) {
  for (int i = 0; i < numInterventions; ++i) {
    const Intervention<Person> &inter = diseaseModel->getPersonIntervention(i);
    p.toggleCompliance(i, inter.willComply(p, &generator));
  }
}

#if ENABLE_DEBUG >= DEBUG_PER_CHARE
  CkPrintf("  Chare %d took %f s to load people\n", thisIndex,
      CkWallTimer() - startTime);
#endif

  // Notify Main
  mainProxy.CharesCreated();
}

People::People(CkMigrateMessage *msg) {}

void People::generatePeopleData() {
  // Init peoples ids and randomly init ages.
  std::uniform_int_distribution<int> age_dist(0, 100);
  int ageIndex = diseaseModel->personAttributes.getAttributeIndex("age");
  for (int i = 0; i < numLocalPeople; i++) {
    Person &p = people[i];

    std::vector<union Data> data = p.getData();
    if (-1 != ageIndex) {
      data[ageIndex].int_b10 = age_dist(generator);
    }


      p.setUniqueId(firstLocalPersonIdx + i);
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
  std::poisson_distribution<int> visit_distance_generator(LOCATION_LAMBDA);

  // Model visit times as uniform.
  std::uniform_int_distribution<int> time_dist(0, DAY_LENGTH);  // in seconds
  std::priority_queue<int, std::vector<int>, std::greater<int> > times;

  // Flip a coin to decide directions in each dimension
  std::uniform_int_distribution<int> dir_gen(0, 1);

  // Calculate minigrid sizes.
  int numLocationsPerPartition = getNumElementsPerPartition(
    numLocations, numLocationPartitions);
  int locationPartitionWidth = synLocalLocationGridWidth;
  int locationPartitionHeight = synLocalLocationGridHeight;
  int locationPartitionGridWidth = synLocationPartitionGridWidth;
#if ENABLE_DEBUG >= DEBUG_BASIC
  if (0 == thisIndex) {
    CkPrintf("location grid at each chare is %d by %d\r\n",
      locationPartitionWidth, locationPartitionHeight);
  }
#endif

  // Choose one location partition for the people in this parition to call home
  int homePartitionIdx = thisIndex % numLocationPartitions;
  int homePartitionX = homePartitionIdx % locationPartitionGridWidth;
  int homePartitionY = homePartitionIdx / locationPartitionGridWidth;
  int homePartitionStartX = homePartitionX * locationPartitionWidth;
  int homePartitionStartY = homePartitionY * locationPartitionHeight;
  int homePartitionNumLocations = getNumLocalElements(
    numLocations, numLocationPartitions, homePartitionIdx);

  // Calculate schedule for each person.
  for (Person &p : people) {
    // Check if person is self isolating.
    int personIdx = p.getUniqueId();

    // Calculate home location
    int localPersonIdx = (personIdx - firstLocationIdx) % homePartitionNumLocations;
    int homeX = homePartitionStartX + localPersonIdx % locationPartitionWidth;
    int homeY = homePartitionStartY + localPersonIdx / locationPartitionWidth;

    p.visitsByDay.resize(numDaysWithDistinctVisits);
    for (std::vector<VisitMessage> &visits : p.visitsByDay) {
      // Get random number of visits for this person.
      int numVisits = num_visits_generator(generator);
      totalVisitsForDay += numVisits;
      // Randomly generate start and end times for each visit,
      // using a priority queue ensures the times are in order.
      for (int j = 0; j < 2 * numVisits; j++) {
        times.push(time_dist(generator));
      }

      totalNumVisits += numVisits;

      // Randomly pick nearby location for person to visit.
      for (int j = 0; j < numVisits; j++) {
        // Generate visit start and end times.
        int visitStart = times.top();
        times.pop();
        int visitEnd = times.top();
        times.pop();
        // Skip empty visits.
        if (visitStart == visitEnd)
          continue;

        // Get number of locations away this person should visit.
        int numHops = std::min(visit_distance_generator(generator),
          synLocationGridWidth + synLocationGridHeight - 2);

        int destinationOffsetX = 0;
        int destinationOffsetY = 0;

        if (numHops != 0) {
          // Calculate maximum hops that can be taken from home location in each
          // direction. (i.e. might be constrained for home locations close to edge)
          int maxHopsNegativeX = std::min(numHops, homeX);
          int maxHopsPositiveX = std::min(numHops,
            synLocationGridWidth - 1 - homeX);
          int maxHopsNegativeY = std::min(numHops, homeY);
          int maxHopsPositiveY = std::min(numHops,
            synLocationGridHeight - 1 - homeY);

          // Choose random number of hops in the X direction.
          std::uniform_int_distribution<int> dist_gen(-maxHopsNegativeX,
            maxHopsPositiveX);
          destinationOffsetX = dist_gen(generator);

          // Travel the remaining hops in the Y direction
          numHops -= std::abs(destinationOffsetX);
          if (numHops != 0) {
            // Choose a random direction between positive and negative
            if (dir_gen(generator) == 0) {
              // Offset positively in Y.
              destinationOffsetY = std::min(numHops, maxHopsPositiveY);
            } else {
              // Offset negatively in Y.
              destinationOffsetY = -std::min(numHops, maxHopsNegativeY);
            }
          }
        }

        // Finally calculate the index of the location to actually visit...
        int destinationX = homeX + destinationOffsetX;
        int destinationY = homeY + destinationOffsetY;

        // ...and translate it from 2D to 1D, respecting the 2D distribution
        // of the locations across partitions
        int partitionX = destinationX / locationPartitionWidth;
        int partitionY = destinationY / locationPartitionHeight;
        int destinationIdx =
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
  std::string scenarioId = getScenarioId(numPeople, numPeoplePartitions,
    numLocations, numLocationPartitions);
  std::ifstream peopleData(scenarioPath + "people.csv");
  std::ifstream peopleCache(scenarioPath + scenarioId + "_people.cache",
    std::ios_base::binary);
  if (!peopleData || !peopleCache) {
    CkAbort("Could not open person data input.");
  }

  // Find starting line for our data through people cache.
  peopleCache.seekg(thisIndex * sizeof(uint64_t));
  uint64_t peopleOffset;
  peopleCache.read(reinterpret_cast<char *>(&peopleOffset), sizeof(uint64_t));
  peopleData.seekg(peopleOffset);

  // Read in from remote file.
  DataReader<Person>::readData(&peopleData, diseaseModel->personDef, &people);
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
  uint64_t *buf =
    reinterpret_cast<uint64_t *>(malloc(sizeof(uint64_t) * numDaysWithDistinctVisits));
  for (int c = 0; c < numLocalPeople; c++) {
    std::vector<uint64_t> *data_pos = &people[c].visitOffsetByDay;
    int curr_id = people[c].getUniqueId();

    // Read in their activity data offsets.
    activityCache.seekg(sizeof(uint64_t) * numDaysWithDistinctVisits
       * (curr_id - firstPersonIdx));
    activityCache.read(reinterpret_cast<char *>(buf),
      sizeof(uint64_t) * numDaysWithDistinctVisits);
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
  #ifdef ENABLE_DEBUG
    int numVisits = 0;
  #endif
  for (Person &person : people) {
    for (int day = 0; day < numDaysWithDistinctVisits; ++day) {
      int nextDaySecs = (day + 1) * DAY_LENGTH;

      // Seek to correct position in file.
      uint64_t seekPos = person
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
      int personId = -1;
      int locationId = -1;
      int visitStart = -1;
      int visitDuration = -1;
      std::tie(personId, locationId, visitStart, visitDuration) =
        DataReader<Person>::parseActivityStream(activityData,
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
        person.visitsByDay[day].emplace_back(locationId, personId, -1,
            visitStart, visitStart + visitDuration, 1.0);
        #ifdef ENABLE_DEBUG
          numVisits++;
        #endif

        std::tie(personId, locationId, visitStart, visitDuration) =
          DataReader<Person>::parseActivityStream(activityData,
              diseaseModel->activityDef, NULL);
      }

      // CkPrintf("  Chare %d: person %d has %u visits on day %d (offset %u)\n",
      //     thisIndex, person.getUniqueId(), person.visitsByDay[day].size(),
      //     day, seekPos);
    }
  }
  #if ENABLE_DEBUG >= DEBUG_VERBOSE
    CkCallback cb(CkReductionTarget(Main, ReceiveVisitsLoadedCount), mainProxy);
    contribute(sizeof(int), &numVisits, CkReduction::sum_int, cb);
  #endif
}

void People::pup(PUP::er &p) {
  p | numLocalPeople;
  p | day;
  p | totalVisitsForDay;
  p | people;
  p | generator;
  p | stateSummaries;

  if (p.isUnpacking()) {
    diseaseModel = globDiseaseModel.ckLocalBranch();
  }
}

void People::SendVisitMessages() {
  // Send activities for each person.
  #if ENABLE_DEBUG >= DEBUG_PER_CHARE
  int minId = numPeople;
  int maxId = 0;
  #endif
  int dayIdx = day % numDaysWithRealData;
  for (const Person &person : people) {
    #if ENABLE_DEBUG >= DEBUG_PER_CHARE
    minId = std::min(minId, person.getUniqueId());
    maxId = std::max(maxId, person.getUniqueId());
    #endif
    for (VisitMessage visitMessage : person.visitsByDay[dayIdx]) {
      visitMessage.personState = person.state;
      visitMessage.transmissionModifier = getTransmissionModifier(person);

      // Interventions may cancel some visits
      if (NULL != visitMessage.deactivatedBy) {
        continue;
      }
      #if ENABLE_DEBUG >= DEBUG_VERBOSE
      totalVisitsForDay++;
      #endif

      // Find process that owns that location
      int locationPartition = getPartitionIndex(visitMessage.locationIdx,
          numLocations, numLocationPartitions, firstLocationIdx);
      // Send off the visit message.
      #ifdef USE_HYPERCOMM
      Aggregator* agg = aggregatorProxy.ckLocalBranch();
      if (agg->visit_aggregator) {
        agg->visit_aggregator->send(locationsArray[locationPartition], visitMessage);
      } else {
      #endif  // USE_HYPERCOMM
        locationsArray[locationPartition].ReceiveVisitMessages(visitMessage);
      #ifdef USE_HYPERCOMM
      }
      #endif  // USE_HYPERCOMM
    }
  }

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
    return person.getValue(diseaseModel->susceptibilityIndex).double_b10;
  } else if (-1 != diseaseModel->infectivityIndex
      && diseaseModel->isInfectious(person.state)) {
    return person.getValue(diseaseModel->infectivityIndex).double_b10;
  }
  return 1.0;
}

void People::ReceiveInteractions(InteractionMessage interMsg) {
  int localIdx = getLocalIndex(interMsg.personIdx, thisIndex, numPeople,
    numPeoplePartitions, firstPersonIdx);

#ifdef ENABLE_DEBUG
  int trueIdx = people[localIdx].getUniqueId();
  if (interMsg.personIdx != trueIdx) {
    CkAbort("Error on chare %d: Person %d's exposure at loc %d recieved by "
        "person %d (local %d)\n",
        thisIndex, interMsg.personIdx, interMsg.locationIdx, trueIdx,
        localIdx);
  }
#endif

  if (0 > localIdx) {
    CkAbort("    Delivered message to person %d (%d on chare %d)\n",
        interMsg.personIdx, localIdx, thisIndex);
  }

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
        && inter.test(person, &generator)) {
      inter.apply(&person);
    }
  }
}

void People::EndOfDayStateUpdate() {
  // Get ready to count today's states
  int totalStates = diseaseModel->getNumberOfStates();
  int offset = (totalStates + 2) * day;
  stateSummaries[offset] = totalVisitsForDay;

  // Handle state transitions at the end of the day.
  int infectiousCount = 0;
  Counter totalExposuresPerDay = 0;
  for (Person &person : people) {
#if ENABLE_DEBUG >= DEBUG_VERBOSE
    totalExposuresPerDay += person.interactions.size();
#endif
    ProcessInteractions(&person);
    UpdateDiseaseState(&person);

    int resultantState = person.state;
    stateSummaries[resultantState + offset + 2]++;
    if (diseaseModel->isInfectious(resultantState)) {
      infectiousCount++;
    }
  }
  stateSummaries[offset + 1] = totalExposuresPerDay;

  // contributing to reduction
  CkCallback cb(CkReductionTarget(Main, ReceiveInfectiousCount), mainProxy);
  contribute(sizeof(int), &infectiousCount, CkReduction::sum_int, cb);
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
  contribute(stateSummaries, CkReduction::sum_int, cb);
}

void People::ProcessInteractions(Person *person) {
  double totalPropensity = 0.0;
  int numInteractions = static_cast<int>(person->interactions.size());
  for (int i = 0; i < numInteractions; ++i) {
    totalPropensity += person->interactions[i].propensity;
  }

  // Detemine whether or not this person was infected...
  double roll = -log(unitDistrib(generator)) / totalPropensity;

  if (roll <= DAY_LENGTH) {
    // ...if they were, determine which interaction was responsible, by
    // chooseing an interaction, with a weight equal to the propensity
    roll = std::uniform_real_distribution<>(0, totalPropensity)(generator);
    double partialSum = 0.0;
    int interactionIdx;
    for (
      interactionIdx = 0; interactionIdx < numInteractions; ++interactionIdx
    ) {
      partialSum += person->interactions[interactionIdx].propensity;
      if (partialSum > roll) {
        break;
      }
    }

    // TODO(jkitson): Save any useful information about the interaction which caused
    // the infection

    // Mark that exposed healthy individuals should make transition at the end
    // of the day.
    if (diseaseModel->isSusceptible(person->state)) {
      person->secondsLeftInState = -1;
    }
  }

  person->interactions.clear();
}

void People::UpdateDiseaseState(Person *person) {
  // Transition to next state or mark the passage of time
  person->secondsLeftInState -= DAY_LENGTH;
  if (person->secondsLeftInState <= 0) {
    // If they have already been infected
    if (person->next_state != -1) {
      person->state = person->next_state;
      std::tie(person->next_state, person->secondsLeftInState) =
        diseaseModel->transitionFromState(person->state, &generator);

    } else {
      // Get which exposed state they should transition to.
      std::tie(person->state, std::ignore) =
        diseaseModel->transitionFromState(person->state, &generator);
      // See where they will transition next.
      std::tie(person->next_state, person->secondsLeftInState) =
        diseaseModel->transitionFromState(person->state, &generator);
    }
  }
}

#ifdef ENABLE_LB
void People::ResumeFromSync() {
  CkCallback cb(CkReductionTarget(Main, peopleLBComplete), mainProxy);
  contribute(cb);
}
#endif  // ENABLE_LB

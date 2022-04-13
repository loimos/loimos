/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "People.h"
#include "Defs.h"
#include "Interaction.h"
#include "DiseaseModel.h"
#include "Person.h"
#include "readers/DataReader.h"

#ifdef USE_HYPERCOMM
  #include "Aggregator.h"
#endif // USE_HYPERCOMM

#include <tuple>
#include <limits>
#include <queue>
#include <cmath>
#include <string>
#include <iostream>
#include <fstream>

std::uniform_real_distribution<> unitDistrib(0,1);
#define ONE_ATTR 1
#define DEFAULT_

People::People() {
  //Must be set to true to make AtSync work
  usesAtSync = true;

  day = 0;
  // generator.seed(thisIndex);
  generator.seed(time(NULL));

  // Initialize disease model
  diseaseModel = globDiseaseModel.ckLocalBranch();

  // Allocate space to summarize the state summaries for every day
  int totalStates = diseaseModel->getNumberOfStates();
  stateSummaries.resize((totalStates + 1) * numDays, 0);

  // Get the number of people assigned to this chare
  numLocalPeople = getNumLocalElements(
    numPeople,
    numPeoplePartitions,
    thisIndex
  );
  
  double startTime = CkWallTimer();

  // Create real or fake people
  if (syntheticRun) {
    // Make a default person and populate people with copies
    Person tmp {0, 0, std::numeric_limits<Time>::max() };
    people.resize(numLocalPeople, tmp);

    // Init peoples ids and randomly init ages.
    int firstPersonIdx = thisIndex * (numPeople / numPeoplePartitions);
    std::uniform_int_distribution<int> age_dist(0, 100);
    for (int p = 0; p < numLocalPeople; p++) {
      Data age;
      age.int_b10 = age_dist(generator);
      std::vector<Data> dataField = { age };

      people[p].uniqueId = firstPersonIdx + p;
      people[p].state = diseaseModel->getHealthyState(dataField);
      // We set persons next state to equal current state to signify
      // that they are not in a disease model progression.
      people[p].next_state = people[p].state;
    } 
  } else {
      int numAttributesPerPerson = 
        DataReader<Person>::getNonZeroAttributes(diseaseModel->personDef);
      for (int p = 0; p < numLocalPeople; p++) {
        people.emplace_back(Person(numAttributesPerPerson,
          0, std::numeric_limits<Time>::max()
        ));
      }

      // Load in people data from file.
      loadPeopleData();
  }
  
  //CkPrintf("  Chare %d took %f s to load people\n", thisIndex,
  //    CkWallTimer() - startTime);

  // Notify Main
  mainProxy.CharesCreated();
  //contribute(CkCallback(CkReductionTarget(Main, CharesCreated), mainProxy));
}

People::People(CkMigrateMessage *msg) {}

/**
 * Loads real people data from file.
 */
void People::loadPeopleData() {  
  std::ifstream peopleData(scenarioPath + "people.csv");
  std::ifstream peopleCache(scenarioPath + scenarioId + "_people.cache", std::ios_base::binary);
  if (!peopleData || !peopleCache) {
    CkAbort("Could not open person data input.");
  }

  // Find starting line for our data through people cache.
  peopleCache.seekg(thisIndex * sizeof(uint32_t));
  uint32_t peopleOffset;
  peopleCache.read((char *) &peopleOffset, sizeof(uint32_t));
  peopleData.seekg(peopleOffset);

  // Read in from remote file.
  DataReader<Person>::readData(&peopleData, diseaseModel->personDef, &people);
  peopleData.close();
  peopleCache.close();

  // Open activity data and cache. 
  std::ifstream activityData(scenarioPath + "visits.csv");
  std::ifstream activityCache(scenarioPath + scenarioId + "_interactions.cache", std::ios_base::binary);
  if (!activityData || !activityCache) {
    CkAbort("Could not open activity input.");
  }

  // Load preprocessing meta data.
  uint32_t *buf = (uint32_t *) malloc(sizeof(uint32_t) * numDaysWithRealData);
  for (int c = 0; c < numLocalPeople; c++) {
    std::vector<uint32_t> *data_pos = &people[c].visitOffsetByDay;
    int curr_id = people[c].uniqueId;

    // Read in their activity data offsets.
    activityCache.seekg(sizeof(uint32_t) * numDaysWithRealData
       * (curr_id - firstPersonIdx));
    activityCache.read((char *) buf, sizeof(uint32_t) * numDaysWithRealData);
    for (int day = 0; day < numDaysWithRealData; day++) {
      data_pos->push_back(buf[day]);
    }
  }
  free(buf);

  // Initialize intial states. (This will move in the DataLoaderPR)
  double isolationCompliance = diseaseModel->getCompilance();
  for (Person &person: people) {
    person.state = diseaseModel->getHealthyState(person.getDataField());
    person.willComply = unitDistrib(generator) < isolationCompliance;
  }

  loadVisitData(&activityData);

  activityData.close();
} 

void People::loadVisitData(std::ifstream *activityData) {
  for (int day = 0; day < numDaysWithRealData; ++day) {
    int nextDaySecs = (day + 1) * DAY_LENGTH;
    for (Person &person: people) {
      
      // Seek to correct position in file.
      uint32_t seekPos = person
        .visitOffsetByDay[day % numDaysWithRealData];
      if (seekPos == EMPTY_VISIT_SCHEDULE) {
        //CkPrintf("No visits on day %d in people chare %d\n", day, thisIndex);
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

      // Seek while same person on same day
      while(personId == person.uniqueId && visitStart < nextDaySecs) {
        // Save visit info
        //CkPrintf("Saving visit data for person %d on day %d\n",
        //  person.uniqueId, day);
        int visitEnd = visitStart + visitDuration;
        if (visitEnd < nextDaySecs) {
          person.visitsByDay[day].emplace_back(locationId, personId, -1,
              visitStart, visitEnd);
        
        } else {
          int tmpDay = day;
          int tmpNextDaySecs = nextDaySecs;
          do {
            person.visitsByDay[tmpDay].emplace_back(locationId, personId, -1,
                visitStart, std::min(visitEnd, tmpNextDaySecs));

            // Split up visits that span multiple days
            visitStart = tmpNextDaySecs;
            tmpDay++;
            tmpNextDaySecs += DAY_LENGTH;
          } while(visitEnd >= tmpNextDaySecs);
        }
        
        std::tie(personId, locationId, visitStart, visitDuration) =
          DataReader<Person>::parseActivityStream(activityData,
              diseaseModel->activityDef, NULL);
      }
    }
  }
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

/**
 * Randomly generates an itinerary (number of visits to random locations)
 * for each person and sends visit messages to locations.
 */ 
void People::SendVisitMessages() {
  totalVisitsForDay = 0;
  if (syntheticRun) {
    SyntheticSendVisitMessages();
  } else {
    RealDataSendVisitMessages();
  }
}

void People::SyntheticSendVisitMessages() {
  // Model number of visits as a poisson distribution.
  std::poisson_distribution<int> num_visits_generator(LOCATION_LAMBDA);

  // Model visit distance as poisson distribution.
  std::poisson_distribution<int> visit_distance_generator(averageDegreeOfVisit);

  // Model visit times as uniform.
  std::uniform_int_distribution<int> time_dist(0, DAY_LENGTH); // in seconds
  std::priority_queue<int, std::vector<int>, std::greater<int> > times;

  // Calculate minigrid sizes.
  int numLocationsPerPartition = getNumElementsPerPartition(
    numLocations,
    numLocationPartitions
  );
  int locationPartitionWidth = synLocalLocationGridWidth;
  int locationPartitionHeight = synLocalLocationGridHeight;
  int locationPartitionGridWidth = synLocationPartitionGridWidth;
  //CkPrintf("location grid at each chare is %d by %d\r\n",
  //  locationPartitionWidth, locationPartitionHeight);

  // Choose one location partition for the people in this parition to call home
  int homePartitionIdx = thisIndex % numLocationPartitions;
  int homePartitionX = homePartitionIdx % locationPartitionGridWidth;
  int homePartitionY = homePartitionIdx / locationPartitionGridWidth;
  int homePartitionStartX = homePartitionX * locationPartitionWidth;
  int homePartitionStartY = homePartitionY * locationPartitionHeight;
  int homePartitionNumLocations = getNumLocalElements(
    numLocations,
    numLocationPartitions,
    homePartitionIdx
  );

  // Calculate schedule for each person.
  for (Person &p : people) {
    // Check if person is self isolating.
    int personIdx = p.uniqueId;
    if (p.isIsolating && diseaseModel->isInfectious(p.state)) {
      continue;
    }

    // Calculate home location
    int localPersonIdx = (personIdx - firstLocationIdx) % homePartitionNumLocations;
    int homeX = homePartitionStartX + localPersonIdx % locationPartitionWidth;
    int homeY = homePartitionStartY + localPersonIdx / locationPartitionWidth;

    // Get random number of visits for this person.
    int numVisits = num_visits_generator(generator);
    totalVisitsForDay += numVisits;
    // Randomly generate start and end times for each visit,
    // using a priority queue ensures the times are in order.
    for (int j = 0; j < 2 * numVisits; j++) {
      times.push(time_dist(generator));
    }

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
        int maxHopsPositiveX = std::min(
            numHops,
            synLocationGridWidth - 1 - homeX
        );
        int maxHopsNegativeY = std::min(numHops, homeY);
        int maxHopsPositiveY = std::min(
            numHops,
            synLocationGridHeight - 1 - homeY
        );
       
        // Choose random number of hops in the X direction.
        std::uniform_int_distribution<int> dist_gen(-maxHopsNegativeX, maxHopsPositiveX);
        destinationOffsetX = dist_gen(generator);

        // Travel the remaining hops in the Y direction
        numHops -= std::abs(destinationOffsetX);
        if (numHops != 0) {
          // Choose a random direction between positive and negative
          std::uniform_int_distribution<int> dir_gen(0, 1);
          
          if (dir_gen(generator) == 0) {
            // Offset positively in Y.
            destinationOffsetY = std::min(numHops, maxHopsPositiveY);
          } else {
            // Offset negatively in Y.
            destinationOffsetY = -std::min(numHops, maxHopsNegativeY);
          }
        }
        // CkPrintf("Home is (%d, %d) (%d %d %d %d))\n", homeX, homeY, maxHopsNegativeX, maxHopsPositiveX, maxHopsNegativeY, maxHopsPositiveY);
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
      //CkPrintf(
      //  "person %d will visit location (%d, %d) with offset (%d,%d)\r\n",
      //  personIdx, destinationX, destinationY, destinationOffsetX, destinationOffsetY);
      //  CkPrintf("(%d, %d) -> %d in partition (%d, %d)\r\n",
      //    destinationX, destinationY, destinationIdx, partitionX, partitionY);

      // Determine which chare tracks this location.
      int locationPartition = getPartitionIndex(
        destinationIdx,
        numLocations,
        numLocationPartitions,
        firstLocationIdx
      );

      // Send off visit message
      VisitMessage visitMsg(destinationIdx, personIdx, p.state, visitStart, visitEnd);
      #ifdef USE_HYPERCOMM
      Aggregator* agg = aggregatorProxy.ckLocalBranch();
      if (agg->visit_aggregator) {
        agg->visit_aggregator->send(locationsArray[locationPartition], visitMsg);
      } else {
      #endif // USE_HYPERCOMM
        locationsArray[locationPartition].ReceiveVisitMessages(visitMsg);
      #ifdef USE_HYPERCOMM
      }
      #endif // USE_HYPERCOMM
    } 
  }
}

void People::RealDataSendVisitMessages() {
  // Send activities for each person.
  for (const Person &person: people) {
    for (VisitMessage visitMessage:
        person.visitsByDay[day % numDaysWithRealData]) {
      visitMessage.personState = person.state;

      // Find process that owns that location
      int locationPartition = getPartitionIndex(visitMessage.locationIdx,
          numLocations, numLocationPartitions, firstLocationIdx);
      // Send off the visit message.
      #ifdef USE_HYPERCOMM
      Aggregator* agg = aggregatorProxy.ckLocalBranch();
      if (agg->visit_aggregator) {
        agg->visit_aggregator->send(locationsArray[locationPartition], visitMessage);
      } else {
      #endif // USE_HYPERCOMM
        locationsArray[locationPartition].ReceiveVisitMessages(visitMessage);
      #ifdef USE_HYPERCOMM
      }
      #endif // USE_HYPERCOMM
    }
  }
}

void People::ReceiveInteractions(InteractionMessage interMsg) {
  int localIdx = getLocalIndex(
    interMsg.personIdx,
    numPeople,
    numPeoplePartitions,
    firstPersonIdx
  );

  // Just concatenate the interaction lists so that we can process all of the
  // interactions at the end of the day
  Person &person = people[localIdx];
  person.interactions.insert(
    person.interactions.end(),
    interMsg.interactions.cbegin(),
    interMsg.interactions.cend()
  );
}

void People::EndOfDayStateUpdate() {
  // Get ready to count today's states
  int totalStates = diseaseModel->getNumberOfStates();
  int offset = (totalStates + 1) * day;
  stateSummaries[offset] = totalVisitsForDay;

  // Handle state transitions at the end of the day.
  int infectiousCount = 0;
  for (Person &person : people) {
    ProcessInteractions(person);
    
    person.EndOfDayStateUpdate(diseaseModel, &generator);
    
    int resultantState = person.state;
    stateSummaries[resultantState + offset + 1]++;
    if (diseaseModel->isInfectious(resultantState)) {
      infectiousCount++;
    }
  }

  // contributing to reduction
  CkCallback cb(CkReductionTarget(Main, ReceiveInfectiousCount), mainProxy);
  contribute(sizeof(int), &infectiousCount, CkReduction::sum_int, cb);
  
  // Get ready for the next day
  day++;
}

void People::SendStats() {
  CkCallback cb(CkReductionTarget(Main, ReceiveStats), mainProxy);
  contribute(stateSummaries, CkReduction::sum_int, cb);
}

void People::ProcessInteractions(Person &person) {
  double totalPropensity = 0.0;
  int numInteractions = (int) person.interactions.size();
  for (int i = 0; i < numInteractions; ++i) {
    totalPropensity += person.interactions[i].propensity;
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
      partialSum += person.interactions[interactionIdx].propensity;
      if (partialSum > roll) {
        break;
      }
    }

    // TODO: Save any useful information about the interaction which caused
    // the infection

    // Mark that exposed healthy individuals should make transition at the end
    // of the day.
    if (diseaseModel->isSusceptible(person.state)) {
      person.secondsLeftInState = -1; 
    }
  }

  person.interactions.clear();
}

#ifdef ENABLE_LB
void People::ResumeFromSync() {
  CkCallback cb(CkReductionTarget(Main, peopleLBComplete), mainProxy);
  contribute(cb);
}
#endif // ENABLE_LB

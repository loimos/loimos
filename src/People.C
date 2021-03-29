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

#include <tuple>
#include <limits>
#include <queue>
#include <cmath>
#include <string>
#include <iostream>
#include <fstream>

std::uniform_real_distribution<> unitDistrib(0,1);
#define NO_ATTRS 0

People::People() {
  newCases = 0;
  day = 0;
  generator.seed(thisIndex);

  // Initialize disease model and identify the healthy state
  diseaseModel = globDiseaseModel.ckLocalBranch();
  int healthyState = diseaseModel->getHealthyState();

  // Get the number of people assigned to this chare
  numLocalPeople = getNumLocalElements(
    numPeople,
    numPeoplePartitions,
    thisIndex
  );
  
  // Create real or fake people
  if (syntheticRun) {
    // Make a default person and populate people with copies
    Person tmp { NO_ATTRS, healthyState, std::numeric_limits<Time>::max() };
    people.resize(numLocalPeople, tmp);
  } else {
      int numAttributesPerPerson = 
        DataReader<Person>::getNonZeroAttributes(diseaseModel->personDef);
      for (int p = 0; p < numLocalPeople; p++) {
        people.emplace_back(Person(numAttributesPerPerson,
          healthyState, std::numeric_limits<Time>::max()
        ));
      }

      // Load in people data from file.
      loadPeopleData();
  }
  
  // Randomly infect people to seed the initial outbreak
  for (Person &person: people) {
    if (unitDistrib(generator) < INITIAL_INFECTIOUS_PROBABILITY) {
      std::tie(person.state, person.secondsLeftInState) = diseaseModel->getExposedState();
      newCases++;
    }
  }
}

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
  activityData = new std::ifstream(scenarioPath + "visits.csv");
  std::ifstream activityCache(scenarioPath + scenarioId + "_interactions.cache", std::ios_base::binary);
  if (!activityData || !activityCache) {
    CkAbort("Could not open activity input.");
  }

  // Load preprocessing meta data.
  uint32_t *buf = (uint32_t *) malloc(sizeof(uint32_t) * numDays);
  for (int c = 0; c < numLocalPeople; c++) {
    std::vector<uint32_t> *data_pos = &people[c].interactionsByDay;
    int curr_id = people[c].uniqueId;

    // Read in their activity data offsets.
    // activityCache.seekg(0);
    activityCache.seekg(sizeof(uint32_t) * numDays * (curr_id - firstPersonIdx));
    activityCache.read((char *) buf, sizeof(uint32_t) * numDays);
    for (int day = 0; day < numDays; day++) {
      data_pos->push_back(buf[day]);
    }
  }
  free(buf);
} 

/**
 * Randomly generates an itinerary (number of visits to random locations)
 * for each person and sends visit messages to locations.
 */ 
void People::SendVisitMessages() {
  if (syntheticRun) {
    SyntheticSendVisitMessages();
  } else {
    RealDataSendVisitMessages();
  }
}

void People::SyntheticSendVisitMessages() {
  int numVisits, personIdx, locationIdx, locationSubset;
  // initialize random number generator for a Poisson distribution
  std::poisson_distribution<int> poisson_dist(LOCATION_LAMBDA);

  // there should be an equal chance of visiting each location
  // and selecting any given time
  std::uniform_int_distribution<int> location_dist(0, numLocations - 1);
  std::uniform_int_distribution<int> time_dist(0, DAY_LENGTH); // in seconds
    
  std::priority_queue<int, std::vector<int>, std::greater<int> > times;

  // generate itinerary for each person
  for (int i = 0; i < numLocalPeople; i++) {
    personIdx = getGlobalIndex(
      i,
      thisIndex,
      numPeople,
      numPeoplePartitions,
      firstPersonIdx
    );
    int personState = people[i].state;

    // getting random number of locations to visit
    numVisits = poisson_dist(generator);
    //CkPrintf("Person %d with %d visits\n",personIdx,visits);
    
    // randomly generate start and end times for each visit,
    // using a priority queue ensures the times are in order
    for (int j = 0; j < 2*numVisits; j++) {
      times.push(time_dist(generator));
    }

    for (int j = 0; j < numVisits; j++) {
      int visitStart = times.top();
      times.pop();
      int visitEnd = times.top();
      times.pop();
      
      // we don't guarentee that these times aren't equal
      // so this is a workaround
      if (visitStart == visitEnd) {
        continue;
      }

      // generate random location to visit
      locationIdx = location_dist(generator);
      //CkPrintf("Person %d visits %d\n",personIdx,locationIdx);
      locationSubset = getPartitionIndex(
        locationIdx,
        numLocations,
        numLocationPartitions,
        firstLocationIdx
      );

      // sending message to location
      VisitMessage visitMsg(locationIdx, personIdx, personState, visitStart, visitEnd);
      locationsArray[locationSubset].ReceiveVisitMessages(visitMsg);
    }
  }
}

void People::RealDataSendVisitMessages() {
  // Send of activities for each person.
  int nextDaySecs = (day + 1) * DAY_LENGTH;
  for (int localPersonId = 0; localPersonId < numLocalPeople; localPersonId++) {
    // Seek to correct position in file.
    uint32_t seekPos = people[localPersonId].interactionsByDay[day];
    if (seekPos == EMPTY_VISIT_SCHEDULE)
      continue;
    activityData->seekg(seekPos, std::ios_base::beg);

    // Start reading
    int personId = -1; 
    int locationId = -1;
    int visitStart = -1;
    int visitDuration = -1;
    std::tie(personId, locationId, visitStart, visitDuration) = DataReader<Person>::parseActivityStream(activityData, diseaseModel->activityDef, NULL);

    // Seek while same person on same day.
    while(personId == people[localPersonId].uniqueId && visitStart < nextDaySecs) {
      // Find process that owns that location.
      int locationSubset = getPartitionIndex(
          locationId,
          numLocations,
          numLocationPartitions,
          firstLocationIdx
      );
      
      // Send off the visit message.
      VisitMessage visitMsg(locationId, personId, people[localPersonId].state, visitStart, visitStart + visitDuration);
      locationsArray[locationSubset].ReceiveVisitMessages(visitMsg);
      std::tie(personId, locationId, visitStart, visitDuration) = DataReader<Person>::parseActivityStream(activityData, diseaseModel->activityDef, NULL);
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

void People::EndofDayStateUpdate() {
  // Handle state transitions at the end of the day.
  int totalStates = diseaseModel->getNumberOfStates();
  std::vector<int> stateSummary(totalStates, 0);
  for (Person &person: people) {
    
    ProcessInteractions(person);
    
    int currState = person.state;
    int secondsLeftInState = person.secondsLeftInState;

    // TODO(iancostello): Move into start of day for visits.
    // Transition to next state or mark the passage of time
    secondsLeftInState -= DAY_LENGTH;
    if (secondsLeftInState <= 0) {
      std::tie(person.state, person.secondsLeftInState) = 
        diseaseModel->transitionFromState(currState, "untreated", &generator);
    
    } else {
      person.secondsLeftInState = secondsLeftInState;
    }

    stateSummary[currState]++;
  }

  // contributing to reduction
  CkCallback cb(CkReductionTarget(Main, ReceiveStats), mainProxy);
  contribute(stateSummary, CkReduction::sum_int, cb);
  day++;
  newCases = 0;
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
    if (person.state == diseaseModel->getHealthyState()) {
      person.secondsLeftInState = -1; 
    }
  }

  person.interactions.clear();
}

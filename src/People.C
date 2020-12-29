/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "People.h"
#include "Defs.h"
#include "DiseaseModel.h"
#include "Person.h"
#include "Attributes.h"
#include "data/DataReader.h"

#include <tuple>
#include <limits>
#include <queue>
#include <string>
#include <iostream>
#include <fstream>

People::People() {
  newCases = 0;
  day = 0;
  generator.seed(thisIndex + 5);

  // Initialize disease model and identify the healthy state
  diseaseModel = globDiseaseModel.ckLocalBranch();
  int healthyState = diseaseModel->getHealthyState();

  // Get the number of people assigned to this chare
  numLocalPeople = getNumLocalElements(
    numPeople,
    numPeoplePartitions,
    thisIndex
  );

  // Make a default person and populate people with copies
  int numAttributesPerPerson = 
    DataReader<Person>::getNonZeroAttributes(diseaseModel->personDef);
  for (int p = 0; p < numLocalPeople; p++) {
    people.push_back(new Person(numAttributesPerPerson,
      healthyState, std::numeric_limits<Time>::max()
    ));
  }

  // Load in people data from file.
  int startingLineIndex = getGlobalIndex(0, thisIndex, numPeople, numPeoplePartitions, PERSON_OFFSET) - PERSON_OFFSET;
  int endingLineIndex = startingLineIndex + numLocalPeople;
  
  std::string line;
  std::ifstream peopleData(scenarioPath + "people.csv");
  std::ifstream peopleCache(scenarioPath + scenarioId + "_people.cache");
  if (!peopleData || !peopleCache) {
    CkAbort("Could not open person data input.");
  }
  // Find starting line for our data through people cache.
  peopleCache.seekg(thisIndex * sizeof(uint32_t));
  uint32_t peopleOffset;
  peopleCache.read((char *) &peopleOffset, sizeof(uint32_t));
  peopleData.seekg(peopleOffset);

  // Read in from remote file.
  DataReader<Person *>::readData(&peopleData, diseaseModel->personDef, &people);
  peopleData.close();
  peopleCache.close();

  // Open activity data and cache. 
  activityData = new std::ifstream(scenarioPath + "interactions.csv", std::ios::binary);
  std::ifstream activityCache(scenarioPath + scenarioId + "_interactions.cache", std::ios::binary);
  if (!activityData || !activityCache) {
    CkAbort("Could not open activity input.");
  }

  // Load preprocessing meta data.
  uint32_t *buf = (uint32_t *) malloc(sizeof(uint32_t) * numDays);
  for (int c = 0; c < numLocalPeople; c++) {
    std::vector<uint32_t> *data_pos = &people[c]->interactionsByDay;
    int curr_id = people[c]->uniqueId;

    // Read in their activity data offsets.
    activityCache.seekg(sizeof(uint32_t) * numDays * (curr_id - firstPersonIdx));
    activityCache.read((char *) buf, sizeof(uint32_t) * numDays);
    for (int day = 0; day < numDays; day++) {
      data_pos->push_back(buf[day]);
    }
  }
  free(buf);
  
  // Randomly infect people to seed the initial outbreak
  std::uniform_real_distribution<> unitDistrib(0,1);
  for (int i = 0; i < people.size(); ++i) {
    if (unitDistrib(generator) < INITIAL_INFECTIOUS_PROBABILITY) {
      people[i]->state = INFECTIOUS;
      people[i]->secondsLeftInState = INFECTION_PERIOD;
      newCases++;
    }
  }
}

/**
 * Randomly generates an itinerary (number of visits to random locations)
 * for each person and sends visit messages to locations.
 */ 
void People::SendVisitMessages() {
  int numVisits, personIdx, locationIdx, locationSubset;

  // initialize random number generator for a Poisson distribution
  std::poisson_distribution<int> poisson_dist(LOCATION_LAMBDA);

  // there should be an equal chance of visiting each location
  // and selecting any given time
  std::uniform_int_distribution<int> location_dist(0, numLocations - 1);
  std::uniform_int_distribution<int> time_dist(0, DAY_LENGTH); // in seconds
    
  std::priority_queue<int, std::vector<int>, std::greater<int> > times;

  // Load iterinary for each person.
  std::string line;
  int current_day_secs = day * (3600 * 24);
  int next_day_secs = (day + 1) * (3600 * 24);
  
  // Send of activities for each person.
  for (int localPersonId = 0; localPersonId < numLocalPeople; localPersonId++) {
    // Seek to correct position in file.
    uint32_t seekPos = people[localPersonId]->interactionsByDay[day];
    if (seekPos == 0xFFFFFFFF)
      continue;
    activityData->seekg(seekPos, std::ios_base::beg);

    // Start reading
    int personId = -1; 
    int locationId = -1;
    int start_time = -1;
    int duration = -1;
    std::tie(personId, locationId, start_time, duration) = DataReader<Person *>::parseActivityStream(activityData, diseaseModel->activityDef, NULL);

    // Seek while same person on same day.
    while(personId == people[localPersonId]->uniqueId && start_time < next_day_secs) {
      // Find process that owns that location.
      locationSubset = getPartitionIndex(
          locationId,
          numLocations,
          numLocationPartitions,
          LOCATION_OFFSET
      );
      // Send off the visit message.
      locationsArray[locationSubset].ReceiveVisitMessages(
        locationId,
        personId,
        people[localPersonId]->state,
        start_time,
        start_time + duration
      );
      std::tie(personId, locationId, start_time, duration) = DataReader<Person *>::parseActivityStream(activityData, diseaseModel->activityDef, NULL);
    }
  }
}

void People::ReceiveInfections(int personIdx) {
  // updating state of a person
  int localIdx = getLocalIndex(
    personIdx,
    numPeople,
    numPeoplePartitions,
    PERSON_OFFSET
  );
  
  // Mark that exposed healthy individuals should make transition at the end
  // of the day.
  if (people[localIdx]->state == diseaseModel->getHealthyState()) {
    people[localIdx]->secondsLeftInState = -1; 
  }
}

void People::EndofDayStateUpdate() {
  int total = 0;

  // Handle state transitions at the end of the day.
  int totalStates = diseaseModel->getNumberOfStates();
  std::vector<int> stateSummary(totalStates, 0);
  for (int i = 0; i < people.size(); i++) {
    int currState = people[i]->state;
    int secondsLeftInState = people[i]->secondsLeftInState;

    // TODO(iancostello): Move into start of day for visits.
    // Transition to next state or mark the passage of time
    secondsLeftInState -= DAY_LENGTH;
    if (secondsLeftInState <= 0) {
      std::tie(people[i]->state, people[i]->secondsLeftInState) = 
        diseaseModel->transitionFromState(currState, "untreated", &generator);
    
    } else {
      people[i]->secondsLeftInState = secondsLeftInState;
    }

    stateSummary[currState]++;
  }

  // contributing to reduction
  CkCallback cb(CkReductionTarget(Main, ReceiveStats), mainProxy);
  contribute(stateSummary, CkReduction::sum_int, cb);
  day++;
  newCases = 0;
}

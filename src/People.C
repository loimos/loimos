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
#include "Attributes.h"

#include <tuple>
#include <limits>
#include <queue>
#include <cmath>

std::uniform_real_distribution<> unitDistrib(0,1);
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
  std::ifstream f(scenarioPath + "people.csv");
  if (!f) {
    CkAbort("Could not open person data input.");
  }
  
  // TODO (iancostello): build an index at preprocessing and seek. 
  for (int i = 0; i <= startingLineIndex; i++) {
    std::getline(f, line);
  }

  // Read in from remote file.
  DataReader<Person *>::readData(&f, diseaseModel->personDef, &people);

  // Open
  activity_stream = new std::ifstream(scenarioPath + "interactions.csv", std::ios::binary);
  if (!activity_stream->is_open()) {
    CkAbort("Could not open activity input.");
  }

  // Load preprocessing meta data.
  std::ifstream data_stream("data_index_activity.csv");
  for (int c = 0; c < numLocalPeople; c++) {
    std::vector<uint32_t> *data_pos = &people[c]->interactionsByDay;
    int curr_id = people[c]->uniqueId;
    
    // Search for line.
    getline(data_stream, line);
    while (!data_stream.eof() && getIntAttribute(&line, 0) != curr_id) {
      getline(data_stream, line);
    }

    // Extra rest of data
    int curr_left = line.find(',') + 1;
    for (int i = curr_left; i < line.size(); i++) {
      if (line.at(i) == ' ') {
        data_pos->push_back(std::stoi(line.substr(curr_left, i)));
        curr_left = i;
      }
    }
  }

  // Clear header.
  std::getline(*activity_stream, line);

  
  // Randomly infect people to seed the initial outbreak
  for (Person &person: people) {
    if (unitDistrib(generator) < INITIAL_INFECTIOUS_PROBABILITY) {
      people[i]->state = INFECTIOUS;
      people[i]->secondsLeftInState = INFECTION_PERIOD;
      newCases++;
    }
  }

  CkPrintf("People chare %d with %d people\n",thisIndex,numLocalPeople);
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
    uint32_t seek_pos = people[localPersonId]->interactionsByDay[day];
    if (seek_pos == 0xFFFFFFFF)
      continue;

    activity_stream->seekg(seek_pos, std::ios_base::beg);
    std::getline(*activity_stream, line);
  
    while (!activity_stream->eof() 
        && getIntAttribute(&line, 4) < next_day_secs
        && getIntAttribute(&line, 1) == people[localPersonId]->uniqueId) {
      int person_id = getIntAttribute(&line, 1);
      int location_id = getIntAttribute(&line, 2);
      int start_time = getIntAttribute(&line, 4);
      int duration = getIntAttribute(&line, 5);

      locationSubset = getPartitionIndex(
          location_id,
          numLocations,
          numLocationPartitions,
          LOCATION_OFFSET
      );

      // if (thisIndex == 0 && localPersonId == 0)
      //   CkPrintf("On day %d, Person %d visited %d on %d at %d for %d\n", day, person_id, location_id, locationSubset, start_time, duration);

      locationsArray[locationSubset].ReceiveVisitMessages(
        location_id,
        person_id,
        people[localPersonId]->state,
        start_time,
        start_time + duration
      );

      if (!activity_stream->eof())
        std::getline(*activity_stream, line);
    }
  }
}

void People::ReceiveInteractions(
  int personIdx,
  const std::vector<Interaction> &interactions
) {
  int localIdx = getLocalIndex(
    personIdx,
    numPeople,
    numPeoplePartitions,
    PERSON_OFFSET
  );

  // Just concatenate the interaction lists so that we can process all of the
  // interactions at the end of the day
  Person *person = people[localIdx];
  person->interactions.insert(
    person.interactions.cend(),
    interactions.cbegin(),
    interactions.cend()
  );
}

void People::EndofDayStateUpdate() {
  // Handle state transitions at the end of the day.
  int totalStates = diseaseModel->getNumberOfStates();
  std::vector<int> stateSummary(totalStates, 0);
  for (Person *person: people) {
    
    ProcessInteractions(person);
    
    int currState = person->state;
    int secondsLeftInState = person->secondsLeftInState;

    // TODO(iancostello): Move into start of day for visits.
    // Transition to next state or mark the passage of time
    secondsLeftInState -= DAY_LENGTH;
    if (secondsLeftInState <= 0) {
      std::tie(person->state, person->secondsLeftInState) = 
        diseaseModel->transitionFromState(currState, "untreated", &generator);
    
    } else {
      person->secondsLeftInState = secondsLeftInState;
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

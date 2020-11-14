/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "People.h"
#include "Defs.h"
#include "DiseaseModel.h"
#include "Attributes.h"

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
  Person tmp { 0, healthyState, std::numeric_limits<Time>::max(), std::vector<uint32_t>() };
  people.resize(numLocalPeople, tmp);

  // Load in people data from file.
  int startingLineIndex = getGlobalIndex(0, thisIndex, numPeople, numPeoplePartitions, PERSON_OFFSET) - PERSON_OFFSET;
  int endingLineIndex = startingLineIndex + numLocalPeople;
  
  std::string line;
  std::ifstream f("small_input/people.csv");
  if (!f) {
    CkAbort("Could not open person data input.");
  }
  
  // TODO (iancostello): build an index at preprocessing and seek. 
  for (int i = 0; i <= startingLineIndex; i++) {
    std::getline(f, line);
  }
    
  for (int i = 0; i < numLocalPeople; i++) {
    std::getline(f, line);
    loadPersonFromCSV(i, &line);
  }

  // Open
  activity_stream = new std::ifstream("small_input/interactions.csv", std::ios::binary);
  if (!activity_stream->is_open()) {
    CkAbort("Could not open activity input.");
  }

  // Load preprocessing meta data.
  std::ifstream data_stream("data_index_activity.csv");
  for (int c = 0; c < numLocalPeople; c++) {
    std::vector<uint32_t> *data_pos = &people[c].interactions_by_day;
    int curr_id = people[c].unique_id;
    
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
        curr_left = 0;
      }
    }
  }

  // Clear header.
  std::getline(*activity_stream, line);

  
  // Randomly infect people to seed the initial outbreak
  std::uniform_real_distribution<> unitDistrib(0,1);
  for (int i = 0; i < people.size(); ++i) {
    if (unitDistrib(generator) < INITIAL_INFECTIOUS_PROBABILITY) {
      people[i].state = INFECTIOUS;
      people[i].secondsLeftInState = INFECTION_PERIOD;
      newCases++;
    }
  }

  CkPrintf("People chare %d with %d people\n",thisIndex,numLocalPeople);
}

void People::loadPersonFromCSV(int personIdx, std::string *data) {
  int attr_index = 0;
  int left_comma = 0;

  // TODO general purpose processor.
  for (int c = 0; c < data->length(); c++) {
    if (data->at(c) == ',') {
      // Completed attribute.
      std::string sub_str = data->substr(left_comma, c);

      //TODO HANDLE THESE ATTRIBUTES WITH PROTOBUF
      if (attr_index == 1) {
        // TODO remove debug
        // if (personIdx == 0)
        //   CkPrintf("ID %d\n", std::stoi(sub_str));
        try {
          people[personIdx].unique_id = std::stoi(sub_str);
        } catch (const std::exception& e) {
          CkPrintf("Could not unpack parse %s on id %d\n", sub_str.c_str(), personIdx);
        }
      }

      left_comma = c + 1;
      attr_index += 1;
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
    uint32_t seek_pos = people[localPersonId].interactions_by_day[day];
    if (seek_pos == 0xFFFFFFFF)
      continue;

    if (seek_pos == 0) {
      printf("Issue!\n");
    }

    activity_stream->seekg(seek_pos, std::ios_base::beg);
    std::getline(*activity_stream, line);
 
    while (!activity_stream->eof() 
        && getIntAttribute(&line, 4) < next_day_secs
        && getIntAttribute(&line, 1) == people[localPersonId].unique_id) {
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

      // CkPrintf("Person %d visited %d on %d at %d for %d\n", person_id, location_id, locationSubset, start_time, duration);
      locationsArray[locationSubset].ReceiveVisitMessages(
        location_id,
        person_id,
        people[localPersonId].state,
        start_time,
        start_time + duration
      );

      if (!activity_stream->eof())
        std::getline(*activity_stream, line);
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
  if (people[localIdx].state == diseaseModel->getHealthyState()) {
    people[localIdx].secondsLeftInState = -1; 
  }
}

void People::EndofDayStateUpdate() {
  int total = 0;

  // Handle state transitions at the end of the day.
  int totalStates = diseaseModel->getNumberOfStates();
  std::vector<int> stateSummary(totalStates, 0);
  for (int i = 0; i < people.size(); i++) {
    int currState = people[i].state;
    int secondsLeftInState = people[i].secondsLeftInState;

    // TODO(iancostello): Move into start of day for visits.
    // Transition to next state or mark the passage of time
    secondsLeftInState -= DAY_LENGTH;
    if (secondsLeftInState <= 0) {
      std::tie(people[i].state, people[i].secondsLeftInState) = 
        diseaseModel->transitionFromState(currState, "untreated", &generator);
    
    } else {
      people[i].secondsLeftInState = secondsLeftInState;
    }

    stateSummary[currState]++;
  }

  // contributing to reduction
  CkCallback cb(CkReductionTarget(Main, ReceiveStats), mainProxy);
  contribute(stateSummary, CkReduction::sum_int, cb);
  day++;
  newCases = 0;
}

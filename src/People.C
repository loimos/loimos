/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "People.h"
#include "Defs.h"
#include "DiseaseModel.h"
#include <tuple>
#include <limits>


#include <queue>

People::People() {
  // Init default values.
  numLocalPeople = getNumLocalElements(numPeople, numPeoplePartitions, thisIndex);
  day = 0; 

  // Initialize disease model and initial healthy states of all individuals.
  diseaseModel = globDiseaseModel.ckLocalBranch();
  int healthy_state = diseaseModel->getHealthyState();
  peopleState.resize(numLocalPeople, std::make_tuple(healthy_state, std::numeric_limits<Time>::max()));
  
  // Init random number generator.
  generator.seed(thisIndex);
  MAX_RANDOM_VALUE = (float)generator.max();
  float random_value;
  
  // Randomly initiate initial infections.
  for(std::vector<std::tuple<int, Time>>::iterator it = peopleState.begin(); it != peopleState.end(); ++it){
    random_value = (float) generator();
    if(random_value / MAX_RANDOM_VALUE < INITIAL_INFECTIOUS_PROBABILITY) {
      *it = diseaseModel->transitionFromState(healthy_state, "untreated", &generator);
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
  std::uniform_int_distribution<int> time_dist(0, 1440); // in minutes
    
  std::priority_queue<int, std::vector<int>, std::greater<int> > times;

  // generate itinerary for each person
  for (int i = 0; i < numLocalPeople; i++) {
    personIdx = getGlobalIndex(
      i,
      thisIndex,
      numPeople,
      numPeoplePartitions
    );

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
        numLocationPartitions
      );

      // sending message to location
      locationsArray[locationSubset].ReceiveVisitMessages(
        personIdx,
        peopleState[i],
        locationIdx,
        visitStart,
        visitEnd
      );
    }
  }
}

void People::ReceiveInfections(int personIdx) {
  // updating state of a person
  int localIdx = getLocalIndex(personIdx, numPeople, numPeoplePartitions);

  // Handle disease transition.
  int currState, timeLeftInState;

  // Mark that exposed healthy individual should make transition at end of day.
  std::tie(currState, timeLeftInState) = peopleState[localIdx];
  if (currState == diseaseModel->getHealthyState()) {
    peopleState[localIdx] = std::make_tuple(currState, -1); 
  }
}

void People::EndofDayStateUpdate() {
  int total = 0;

  // Handle state transitions at the end of the day.
  int totalStates = diseaseModel->getNumberOfStates();
  std::vector<int> stateSummary(totalStates, 0);
  for(int i = 0; i < peopleState.size(); i++) {
    std::tuple<int,int> current = peopleState[i];
    int currState, timeLeftInState;
    std::tie(currState, timeLeftInState) = current;

    // TODO(iancostello): Move into start of day for visits.
    // Transition to newstate or decrease time.
    timeLeftInState -= (3600) * 24; // One day in seconds
    if (timeLeftInState <= 0) {
      peopleState[i] = diseaseModel->transitionFromState(currState, "untreated", &generator);
    } else {
      peopleState[i] = std::make_tuple(currState, timeLeftInState);
    }

    // Counting people by state.
    stateSummary[currState] += 1;
  }

  // Contribute the result to the reductiontarget cb.
  CkCallback cb(CkReductionTarget(Main, ReceiveStats), mainProxy);
  contribute(stateSummary, CkReduction::sum_int, cb);
}

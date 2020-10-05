/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "Person.h"
#include "People.h"
#include "Defs.h"
#include "DiseaseModel.h"

#include <tuple>
#include <limits>
#include <queue>

People::People() {
  newCases = 0;
  day = 0;
  generator.seed(thisIndex);
  
  // getting number of people assigned to this chare
  numLocalPeople = getNumLocalElements(
    numPeople,
    numPeoplePartitions,
    thisIndex
  );
  Person tmp { SUSCEPTIBLE, 0 };
  people.resize(numLocalPeople, tmp);
  
  // randomnly choosing people as infectious
  std::uniform_real_distribution<> unitDistrib(0,1);
  for (int i = 0; i < people.size(); ++i) {
    if (unitDistrib(generator) < INITIAL_INFECTIOUS_PROBABILITY) {
      people[i].state = INFECTIOUS;
      people[i].nextStateDay = INFECTION_PERIOD;
      newCases++;
    }
  }

  // CkPrintf("People chare %d with %d people\n",thisIndex,numLocalPeople);
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
    int personstate = people[i].state;

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
        locationIdx,
        personIdx,
        personState,
        visitStart,
        visitEnd
      );
    }
  }
}

void People::ReceiveInfections(int personIdx) {
  // updating state of a person
  int localIdx = getLocalIndex(
    personIdx,
    numPeople,
    numPeoplePartitions
  );
  
  // Handle disease transition.
  int currState, timeLeftInState;

  // Mark that exposed healthy individual should make transition at end of day.
  std::tie(currState, timeLeftInState) = peopleState[localIdx];
  if (currState == diseaseModel->getHealthyState()) {
    peopleState[localIdx] = std::make_tuple(currState, -1); 
  }
  
  // Not sure where this state is supposed to come from...
  //if(state) peopleState[localIdx] = state;
  //CkPrintf("Partition %d - Person %d state %d\n",thisIndex,personIdx,state);
}

void People::EndofDayStateUpdate() {
  // counting infected people
  for(int i = 0; i < people.size(); ++i) {
    switch(people[i].state) {
      case SUSCEPTIBLE:
        break;
      
      case EXPOSED:
        if(day > people[i].nextStateDay) {
          newCases++;
          people[i].nextStateDay = day + INFECTION_PERIOD;
          people[i].state = INFECTIOUS;
        }
        break;
      
      case INFECTIOUS:
        if(day > people[i].nextStateDay) {
          people[i].state = RECOVERED;
        }
        break;
      
      case RECOVERED:
        break;
    }
  }

  //PrintstateCounts();

  // contributing to reduction
  CkCallback cb(CkReductionTarget(Main, ReceiveStats), mainProxy);
  contribute(stateSummary, CkReduction::sum_int, cb);
}

void People::PrintStateCounts() {
  int counts[NUMBER_STATES] {0};
  for(Person p : people)
    counts[p.state]++;

  CkPrintf(
    "partion %d has S=%d E=%d I=%d R=%d\r\n",
    thisIndex,
    counts[SUSCEPTIBLE],
    counts[EXPOSED],
    counts[INFECTIOUS],
    counts[RECOVERED]
  );
}

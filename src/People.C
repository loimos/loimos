/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "Person.h"
#include "People.h"
#include "Defs.h"

#include <queue>

People::People() {
  newCases = 0;
  day = 0;
  
  // getting number of people assigned to this chare
  numLocalPeople = getNumLocalElements(
    numPeople,
    numPeoplePartitions,
    thisIndex
  );
  peopleState.resize(numLocalPeople, SUSCEPTIBLE);
  peopleDay.resize(numLocalPeople, 0);
  generator.seed(thisIndex);
  
  // randomnly choosing people as infectious
  std::uniform_real_distribution<> unitDistrib(0,1);
  for (int i = 0; i < peopleState.size(); ++i) {
    if (unitDistrib(generator) < INITIAL_INFECTIOUS_PROBABILITY) {
      peopleState[i] = INFECTIOUS;
      peopleDay[i] = INFECTION_PERIOD;
      newCases++;
    }
  }

  // CkPrintf("People chare %d with %d people\n",thisIndex,numLocalPeople);
}

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
    char personState = peopleState[i];

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
  /*
  CkPrintf(
    "recieved infection message for person %d in partition %d\r\n",
    personIdx,
    thisIndex
  );
  */
  
  if (SUSCEPTIBLE == peopleState[localIdx]) {
    peopleState[localIdx] = EXPOSED;
    peopleDay[localIdx] = day + INCUBATION_PERIOD;
  }

  // Not sure where this state is supposed to come from...
  //if(state) peopleState[localIdx] = state;
  //CkPrintf("Partition %d - Person %d state %d\n",thisIndex,personIdx,state);
}

void People::EndofDayStateUpdate() {
  // counting infected people
  for(int i = 0; i < peopleState.size(); ++i) {
    switch(peopleState[i]) {
      case SUSCEPTIBLE:
        break;
      
      case EXPOSED:
        if(day > peopleDay[i]) {
          newCases++;
          peopleDay[i] = day + INFECTION_PERIOD;
          peopleState[i] = INFECTIOUS;
        }
        break;
      
      case INFECTIOUS:
        if(day > peopleDay[i]) {
          peopleState[i] = RECOVERED;
        }
        break;
      
      case RECOVERED:
        break;
    }
  }

  //PrintStateCounts();

  // contributing to reduction
  CkCallback cb(CkReductionTarget(Main, ReceiveStats), mainProxy);
  contribute(sizeof(int), &newCases, CkReduction::sum_int, cb);
  day++;
  newCases = 0;
}

void People::PrintStateCounts() {
  int counts[NUMBER_STATES] {0};
  for(char state : peopleState)
    counts[state]++;

  CkPrintf(
    "partion %d has S=%d E=%d I=%d R=%d\r\n",
    thisIndex,
    counts[SUSCEPTIBLE],
    counts[EXPOSED],
    counts[INFECTIOUS],
    counts[RECOVERED]
  );
}

/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "People.h"
#include "Defs.h"

#include <queue>

People::People() {
  float value;
  int cont = 0;
  newCases = 0;
  // getting number of people assigned to this chare
  numLocalPeople = getNumLocalElements(
    numPeople,
    numPeoplePartitions,
    thisIndex
  );
  peopleState.resize(numLocalPeople, SUSCEPTIBLE);
  peopleDay.resize(numLocalPeople, 0);
  generator.seed(thisIndex);
  MAX_RANDOM_VALUE = (float)generator.max();
  // randomnly choosing people as infectious
  for (std::vector<char>::iterator it = peopleState.begin(); it != peopleState.end(); ++it) {
    value = (float)generator();
    if(value/MAX_RANDOM_VALUE < INITIAL_INFECTIOUS_PROBABILITY){
      *it = INFECTIOUS;
      peopleDay[cont] = INFECTION_PERIOD;
      newCases++;
    }
    cont++;
  }
  day = 0;
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
  peopleState[localIdx] = EXPOSED;
  peopleDay[localIdx] = day + INCUBATION_PERIOD;

  // Not sure where this state is supposed to come from...
  //if(state) peopleState[localIdx] = state;
  //CkPrintf("Partition %d - Person %d state %d\n",thisIndex,personIdx,state);
}

void People::EndofDayStateUpdate() {
  int cont = 0;
  // counting infected people
  for(std::vector<char>::iterator it = peopleState.begin() ; it != peopleState.end(); ++it) {
    switch(*it) {
      case SUSCEPTIBLE:
        break;
      case EXPOSED:
        if(day > peopleDay[cont]) {
          newCases++;
          peopleDay[cont] = day + INFECTION_PERIOD;
          peopleState[cont] = INFECTIOUS;
        }
        break;
      case INFECTIOUS:
        if(day > peopleDay[cont]) {
          peopleState[cont] = RECOVERED;
        }
        break;
      case RECOVERED:
        break;
    }
    cont++;
  }

  // contributing to reduction
  CkCallback cb(CkReductionTarget(Main, ReceiveStats), mainProxy);
  contribute(sizeof(int), &newCases, CkReduction::sum_int, cb);
  day++;
  newCases = 0;
}

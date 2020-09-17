/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "People.h"
#include "Defs.h"

People::People() {
  float value;
  int cont = 0;
  newCases = 0;
  // getting number of people assigned to this chare
  numLocalPeople = getNumLocalElements(numPeople, numPeoplePartitions, thisIndex);
  peopleState.resize(numLocalPeople, SUSCEPTIBLE);
  peopleDay.resize(numLocalPeople, 0);
  generator.seed(thisIndex);
  MAX_RANDOM_VALUE = (float)generator.max();
  // randomnly choosing people as infectious
  for(std::vector<char>::iterator it = peopleState.begin(); it != peopleState.end(); ++it){
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
  int visits, personIdx, locationIdx, locationSubset;

  // initialize random number generator for a Poisson distribution
  std::poisson_distribution<int> poisson_dist(LOCATION_LAMBDA);

  // initialize random number generator for a uniform distribution
  std::uniform_int_distribution<int> uniform_dist(0,numLocations-1);
 
  // generate itinerary for each person
  for(int i=0; i<numLocalPeople; i++) {
    personIdx = getGlobalIndex(i, thisIndex, numPeople, numPeoplePartitions);

    // getting random number of locations to visit
    visits = poisson_dist(generator);
    //CkPrintf("Person %d with %d visits\n",personIdx,visits);
    for(int j=0; j<visits; j++) {

      // generate random location to visit
      locationIdx = uniform_dist(generator);
      //CkPrintf("Person %d visits %d\n",personIdx,locationIdx);
      locationSubset = getPartitionIndex(locationIdx, numLocations, numLocationPartitions);

      // sending message to location
      locationsArray[locationSubset].ReceiveVisitMessages(personIdx, peopleState[i], locationIdx);
    }
  }
}

void People::ReceiveInfections(int personIdx) {
  // updating state of a person
  int localIdx = getLocalIndex(personIdx, numPeople, numPeoplePartitions);
  peopleState[localIdx] = EXPOSED;
  peopleDay[localIdx] = day + INCUBATION_PERIOD;
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

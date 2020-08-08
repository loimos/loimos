/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "People.h"
#include "Defs.h"

People::People() {
  // getting number of people assigned to this chare
  numLocalPeople = getNumLocalElems(numPeople, numPeoplePartitions, thisIndex);
  peopleState.resize(numLocalPeople, HEALTHY);
  
  // CkPrintf("People chare %d with %d people\n",thisIndex,numLocalPeople);
}

void People::SendVisitMessages() {
  int visits, personIdx, locationIdx, locationSubset;

  // initialize random number generator for a Poisson distribution
  std::default_random_engine generator;
  std::poisson_distribution<int> poisson_dist(LOCATION_LAMBDA);

  // initialize random number generator for a uniform distribution
  std::uniform_int_distribution<int> uniform_dist(0,numLocations-1);
 
  // generate itinerary for each person
  for(int i=0; i<numLocalPeople; i++) {
    personIdx = getGlobalIndex(i, thisIndex, numPeople, numPeoplePartitions);

    // getting random number of locations to visit
    visits = poisson_dist(generator);
    // CkPrintf("Person %d with %d visits\n",personIdx,visits);
    for(int j=0; j<visits; j++) {

      // generate random location to visit
      locationIdx = uniform_dist(generator);
      locationSubset = getContainerIndex(locationIdx, numLocations, numLocationPartitions);

      // sending message to location
      locationsArray[locationSubset].ReceiveVisitMessages(personIdx, locationIdx);
    }
  }
}

void People::ReceiveInfections(int personIdx, char state) {
  // updating state of a person
  int localIdx = getLocalIndex(personIdx, numPeople, numPeoplePartitions);

  peopleState[localIdx] = state;
  // CkPrintf("Person %d state %d\n",personIdx,state);
}

void People::EndofDayStateUpdate() {
  int total = 0;
  // counting infected people
  for(std::vector<char>::iterator it = peopleState.begin() ; it != peopleState.end(); ++it)
    if(*it == INFECTED) total++;

  // contributing to reduction
  CkCallback cb(CkReductionTarget(Main, ReceiveStats), mainProxy);
  contribute(sizeof(int), &total, CkReduction::sum_int, cb);
}

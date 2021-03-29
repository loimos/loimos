/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "../loimos.decl.h"
#include "SyntheticSchedule.h"
#include "../Person.h"

#include <random>
#include <vector>
#include <queue>

const double LOCATION_LAMBDA = 5.2;

void SyntheticSchedule::setSeed(const int seed) {
  this->seed = seed;
  generator.seed(seed);
}

void SyntheticSchedule::sendVisitMessages(
  const std::vector<Person> &people,
  int peopleChareIndex
) {
  int numVisits, personIdx, locationIdx, locationSubset;
  // initialize random number generator for a Poisson distribution
  std::poisson_distribution<int> poisson_dist(LOCATION_LAMBDA);

  // there should be an equal chance of visiting each location
  // and selecting any given time
  std::uniform_int_distribution<int> location_dist(0, numLocations - 1);
  std::uniform_int_distribution<int> time_dist(0, DAY_LENGTH); // in seconds
    
  std::priority_queue<int, std::vector<int>, std::greater<int> > times;

  // generate itinerary for each person
  int numLocalPeople = (int) people.size();
  for (int i = 0; i < numLocalPeople; i++) {
    personIdx = getGlobalIndex(
      i,
      peopleChareIndex,
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

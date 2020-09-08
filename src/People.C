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

People::People() {
  float value;
  int cont = 0;
  newCases = 0;
  // getting number of people assigned to this chare
  numLocalPeople = getNumLocalElements(numPeople, numPeoplePartitions, thisIndex);
  peopleState.resize(numLocalPeople, std::make_tuple(HEALTHY, __INT_MAX__));
  
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

void People::ReceiveInfections(int personIdx, bool trigger_infection) {
  // updating state of a person
  int localIdx = getLocalIndex(personIdx, numPeople, numPeoplePartitions);

  // Handle disease transition.
  DiseaseModel* diseaseModel = globDiseaseModel.ckLocalBranch();
  int currState, timeLeftInState;

  // TODO(iancostello): Don't make the state update here.
  std::tie(currState, timeLeftInState) = peopleState[localIdx];
  if (trigger_infection && currState == HEALTHY) {
    peopleState[localIdx] = diseaseModel->transitionFromState(currState, "untreated", generator);
  }
  //CkPrintf("Partition %d - Person %d state %d\n",thisIndex,personIdx,state);
}

void People::EndofDayStateUpdate() {
  int total = 0;
  // CProxy_DiseaseModel diseaseModelProxy = CProxy_DiseaseModel::ckNew("");
  DiseaseModel* diseaseModel = globDiseaseModel.ckLocalBranch();
  // CkPrintf("Init State %d", diseaseModel->getUninfectedState());

  // for(std::vector<std::tuple<int,int>>::iterator it = peopleState.begin() ; it != peopleState.end(); ++it) {
  int totalStates = diseaseModel->getTotalStates();
  std::vector<int> stateSummary(totalStates, 0);
  for(int i = 0; i < peopleState.size(); i++) {
    std::tuple<int,int> current = peopleState[i];
    int currState, timeLeftInState;
    std::tie(currState, timeLeftInState) = current;

    // TODO(iancostello): Move into start of day for visits.
    // Transition to new state or decrease time.
    timeLeftInState -= (3600) * 24; // One day in seconds
    if (timeLeftInState <= 0) {
      peopleState[i] = diseaseModel->transitionFromState(currState, "untreated", generator);
    } else {
      peopleState[i] = std::make_tuple(currState, timeLeftInState);
    }
    

    // counting infected people
    stateSummary[currState] += 1;
  }

  // Contribute the result to the reductiontarget cb.
  CkCallback cb(CkReductionTarget(Main, ReceiveStats), mainProxy);
  contribute(stateSummary, CkReduction::sum_int, cb);
}

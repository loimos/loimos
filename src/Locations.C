/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "Locations.h"
#include "Defs.h"
#include <time.h>

Locations::Locations() {
	// getting number of locations assigned to this chare
	numLocalLocations = getNumLocalElems(numLocations, numLocationSubsets, thisIndex);
	locState.resize(numLocalLocations);
	srand((unsigned) time(NULL));
}

void Locations::ReceiveVisitMessages(int personIdx, int locationIdx) {
	// adding person to location visit list
	int localLocIdx = getLocalIndex(locationIdx,numLocations,numLocationSubsets);
	locState[localLocIdx].insert(personIdx);
	//CkPrintf("Location %d localIdx %d visited by person %d\n",locationIdx,localLocIdx,personIdx);
}

void Locations::ComputeInteractions() {
	int peopleSubsetIdx;
	char state;
	// traverses list of locations
	for(std::vector<std::set<int> >::iterator locIter = locState.begin() ; locIter != locState.end(); ++locIter){
		for(std::set<int>::iterator it = locIter->begin() ; it != locIter->end(); ++it){
			// randomly selecting people to get infected
			if((float)rand()/RAND_MAX < INFECTION_PROBABILITY)
				state = INFECTED;
			else
				state = HEALTHY;
			peopleSubsetIdx = getContainerIndex(*it,numPeople,numPeopleSubsets);
			peopleArray[peopleSubsetIdx].UpdateDiseaseState(*it,state);
		}
	}
}


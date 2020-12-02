/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "Location.h"
#include "People.h"
#include "Event.h"
#include "Defs.h"
#include "DiseaseModel.h"

#include <random>
#include <set>
#include <cmath>
#include <algorithm>

std::uniform_real_distribution<> Location::unitDistrib(0,1);

void Location::addEvent(Event e) {
  events.push(e);
}

void Location::processEvents(
  std::default_random_engine *generator,
  const DiseaseModel *diseaseModel
) {
  std::vector<Event> *arrivals;
  Event curEvent;

  while (!events.empty()) {
    curEvent = events.top();
    events.pop();

    if (diseaseModel->isSusceptible(curEvent.personState)) {
      arrivals = &susceptibleArrivals;

    } else if (diseaseModel->isInfectious(curEvent.personState)) {
      arrivals = &infectiousArrivals;

    // If a person can niether infect other people nor be infected themself,
    // we can just ignore their comings and goings
    } else {
      continue;
    }

    if (ARRIVAL == curEvent.type) {
      arrivals->push_back(curEvent);
      std::push_heap(arrivals->begin(), arrivals->end(), Event::greaterPartner);

    } else if (DEPARTURE == curEvent.type) {
      // Remove the arrival event corresponding to this departure 
      std::pop_heap(arrivals->begin(), arrivals->end(), Event::greaterPartner);
      arrivals->pop_back();

      onDeparture(generator, diseaseModel, curEvent);
    }
  }
}

// Simple dispatch to the susceptible/infectious depature handlers
inline void Location::onDeparture(
  std::default_random_engine *generator,
  const DiseaseModel *diseaseModel,
  const Event& departure
) {
  if (diseaseModel->isSusceptible(departure.personState)) {
    onSusceptibleDeparture(generator, diseaseModel, departure);

  } else if (diseaseModel->isInfectious(departure.personState)) {
    onInfectiousDeparture(generator, diseaseModel, departure);
  } 
}

void Location::onSusceptibleDeparture(
  std::default_random_engine *generator,
  const DiseaseModel *diseaseModel,
  const Event& susceptibleDeparture
) {
  double logProbNotInfected = 0.0;
  for (Event infectiousArrival: infectiousArrivals) {
    // Every infectious person contributes to the change a susceptible person
    // is infected
    logProbNotInfected += diseaseModel->getLogProbNotInfected(
      susceptibleDeparture, infectiousArrival
    );

    registerPotentialInfection(
      diseaseModel,
      susceptibleDeparture,
      infectiousArrival,
      infectiousArrival.scheduledTime,
      susceptibleDeparture.scheduledTime
    ); 
  }

  // We want the probability of infection, so we need to 
  // invert probNotInfected
  double prob = exp(logProbNotInfected);
  double roll = unitDistrib(*generator);
  if (roll > prob) {
    infect(susceptibleDeparture.personIdx);
  }
}

void Location::onInfectiousDeparture(
  std::default_random_engine *generator,
  const DiseaseModel *diseaseModel,
  const Event& infectiousDeparture
) {

  // Each susceptible person has a chance of being infected by any given
  // infectious person
  for (Event susceptibleArrival : susceptibleArrivals) {
    // We want the probability of infection, so we need to 
    // invert probNotInfected
    double prob = exp(diseaseModel->getLogProbNotInfected(
      susceptibleArrival, infectiousDeparture
    ));

    double roll = unitDistrib(*generator);
    if (roll > prob) { 
      infect(susceptibleArrival.personIdx);
    }
    
    registerPotentialInfection(
      diseaseModel,
      susceptibleArrival,
      infectiousDeparture,
      susceptibleArrival.scheduledTime,
      infectiousDeparture.scheduledTime
    ); 
  } 
}

<<<<<<< HEAD
// Simple helper function which infects a given person with a given
// probability (we handle this here rather since this is a chare class,
// and so we have access to peopleArray and the like)
inline void Location::infect(int personIdx) const {
  int peoplePartitionIdx = getPartitionIndex(
    personIdx,
    numPeople,
    numPeoplePartitions
  );

  peopleArray[peoplePartitionIdx].ReceiveInfections(personIdx);
  /*
  CkPrintf(
    "sending infection message to person %d in partition %d\r\n",
    personIdx,
    peoplePartitionIdx
  );
  */
=======
inline void Location::registerPotentialInfection(
  DiseaseModel *diseaseModel,
  Event susceptibleEvent,
  Event infectiousEvent,
  int startTime,
  int endTime
) {
  double propensity = diseaseModel->getPropensity(
    susceptibleEvent.personState,
    susceptibleEvent.personState,
    startTime,
    endTime
  );
  PotentialInfection infection {
    propensity,
    infectiousEvent.personIdx,
    infectiousEvent.personState,
    startTime,
    endTime
  };

  // Note that this will create a new vector if this is the first potential
  // infection for the susceptible person in question
  potentialInfections[susceptibleEvent.personIdx].push_back(infection);
>>>>>>> now building up list of all potential infections for each susceptible person
}

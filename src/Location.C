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
#include <vector>
#include <cmath>
#include <algorithm>

void Location::addEvent(Event e) {
  events.push(e);
}

void Location::processEvents(const DiseaseModel *diseaseModel) {
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

      onDeparture(diseaseModel, curEvent);
    }
  }

  interactions.clear();
}

// Simple dispatch to the susceptible/infectious depature handlers
inline void Location::onDeparture(
  const DiseaseModel *diseaseModel,
  const Event& departure
) {
  if (diseaseModel->isSusceptible(departure.personState)) {
    onSusceptibleDeparture(diseaseModel, departure);

  } else if (diseaseModel->isInfectious(departure.personState)) {
    onInfectiousDeparture(diseaseModel, departure);
  } 
}

void Location::onSusceptibleDeparture(
  const DiseaseModel *diseaseModel,
  const Event& susceptibleDeparture
) {
  double logProbNotInfected = 0.0;
  for (Event infectiousArrival: infectiousArrivals) {
    registerInteraction(
      diseaseModel,
      susceptibleDeparture,
      infectiousArrival,
      infectiousArrival.scheduledTime,
      susceptibleDeparture.scheduledTime
    ); 
  }

  sendInteractions(susceptibleDeparture.personIdx);
}

void Location::onInfectiousDeparture(
  const DiseaseModel *diseaseModel,
  const Event& infectiousDeparture
) {

  // Each susceptible person has a chance of being infected by any given
  // infectious person
  for (Event susceptibleArrival : susceptibleArrivals) {
    registerInteraction(
      diseaseModel,
      susceptibleArrival,
      infectiousDeparture,
      susceptibleArrival.scheduledTime,
      infectiousDeparture.scheduledTime
    ); 
  } 
}

inline void Location::registerInteraction(
  const DiseaseModel *diseaseModel,
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
  Interaction infection {
    propensity,
    infectiousEvent.personIdx,
    infectiousEvent.personState,
    startTime,
    endTime
  };

  // Note that this will create a new vector if this is the first potential
  // infection for the susceptible person in question
  interactions[susceptibleEvent.personIdx].push_back(infection);
}

// Simple helper function which send the list of interactions with the
// specified person to the appropriate People chare
inline void Location::sendInteractions(int personIdx) {
  int peoplePartitionIdx = getPartitionIndex(
    personIdx,
    numPeople,
    numPeoplePartitions
  );

  std::vector<Interaction> &personInteractions = interactions[personIdx];
  peopleArray[peoplePartitionIdx].ReceiveInteractions(
    personIdx,
    (int) personInteractions.size(),
    &personInteractions[0]
  );
  /*
  CkPrintf(
    "sending infection message to person %d in partition %d\r\n",
    personIdx,
    peoplePartitionIdx
  );
  */
}

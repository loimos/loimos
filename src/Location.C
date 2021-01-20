/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * Defines a single physical location that a person may visit. 
 */ 

#include "loimos.decl.h"
#include "Location.h"
#include "People.h"
#include "Event.h"
#include "Defs.h"
#include "DiseaseModel.h"
#include "ContactModel.h"

#include <random>
#include <vector>
#include <cmath>
#include <algorithm>

Location::Location(int numAttributes) {
  this->locationData = (union Data *) malloc(numAttributes * sizeof(union Data));
}

Location::~Location() {
  free(this->locationData);
}

// DataInterface overrides. 
void Location::setUniqueId(int idx) {
    this->uniqueId = idx;
}
union Data *Location::getDataField() {
    return this->locationData;
}

void Location::addEvent(Event e) {
  events.push(e);
}

void Location::processEvents(
  const DiseaseModel *diseaseModel,
  ContactModel *contactModel
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

      onDeparture(diseaseModel, contactModel, curEvent);
    }
  }

  interactions.clear();
}

// Simple dispatch to the susceptible/infectious depature handlers
inline void Location::onDeparture(
  const DiseaseModel *diseaseModel,
  ContactModel *contactModel,
  const Event& departure
) {
  if (diseaseModel->isSusceptible(departure.personState)) {
    onSusceptibleDeparture(diseaseModel, contactModel, departure);

  } else if (diseaseModel->isInfectious(departure.personState)) {
    onInfectiousDeparture(diseaseModel, contactModel, departure);
  } 
}

void Location::onSusceptibleDeparture(
  const DiseaseModel *diseaseModel,
  ContactModel *contactModel,
  const Event& susceptibleDeparture
) {
  // Each infectious person at this location might have infected this
  // susceptible person
  for (const Event &infectiousArrival: infectiousArrivals) {
    registerInteraction(
      diseaseModel,
      contactModel,
      susceptibleDeparture,
      infectiousArrival,
      // The start time is whichever arrival happened later
      std::max(
        infectiousArrival.scheduledTime,
        susceptibleDeparture.partnerTime
      ),
      susceptibleDeparture.scheduledTime
    ); 
  }

  sendInteractions(susceptibleDeparture.personIdx);
}

void Location::onInfectiousDeparture(
  const DiseaseModel *diseaseModel,
  ContactModel *contactModel,
  const Event& infectiousDeparture
) {
  // Each susceptible person at this location might have been infected by this
  // infectious person
  for (const Event &susceptibleArrival : susceptibleArrivals) {
    registerInteraction(
      diseaseModel,
      contactModel,
      susceptibleArrival,
      infectiousDeparture,
      // The start time is whichever arrival happened later
      std::max(
        susceptibleArrival.scheduledTime,
        infectiousDeparture.partnerTime
      ),
      infectiousDeparture.scheduledTime
    ); 
  } 
}

inline void Location::registerInteraction(
  const DiseaseModel *diseaseModel,
  ContactModel *contactModel,
  const Event &susceptibleEvent,
  const Event &infectiousEvent,
  int startTime,
  int endTime
) {
  if (!contactModel->madeContact(susceptibleEvent, infectiousEvent)) {
    return;
  }

  double propensity = diseaseModel->getPropensity(
    susceptibleEvent.personState,
    infectiousEvent.personState,
    startTime,
    endTime
  );

  // Note that this will create a new vector if this is the first potential
  // infection for the susceptible person in question
  interactions[susceptibleEvent.personIdx].emplace_back(
    propensity,
    infectiousEvent.personIdx,
    infectiousEvent.personState,
    startTime,
    endTime
  );
}

// Simple helper function which send the list of interactions with the
// specified person to the appropriate People chare
inline void Location::sendInteractions(int personIdx) {
  int peoplePartitionIdx = getPartitionIndex(
    personIdx,
    numPeople,
    numPeoplePartitions
  );

  peopleArray[peoplePartitionIdx].ReceiveInteractions(
    personIdx,
    interactions[personIdx]
  );

  /*  
  CkPrintf(
    "sending %d interactions to person %d in partition %d\r\n",
    (int) interactions[personIdx].size(),
    personIdx,
    peoplePartitionIdx
  );
  */
  
  // Free up space where we were storing interactions data. This also prevents
  // interactions from being sent multiple times if this person has multiple
  // visits to this location
  interactions.erase(personIdx);
}

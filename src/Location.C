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
#include "contact_model/ContactModel.h"

#include <random>
#include <vector>
#include <cmath>
#include <algorithm>

Location::Location(std::shared_ptr<aggregator_t> aggregator, int numAttributes, int uniqueIdx, std::default_random_engine *generator) : uniform_dist(0, 1) {
  this->aggregator = aggregator;
  if (numAttributes != 0) {
    this->locationData.resize(numAttributes);
  }
  day = 0;
  this->generator = generator;
  unitDistrib = std::uniform_real_distribution<>(0.0,1.0);

  // Determine if this location should seed the disease.
  if (syntheticRun) {
    // For synthetic runs start seed in corner.
    // Determine grid size in each corner s.t. randomly selecting 50%
    // of these locations will result
    int seedSize = 
      std::max((int) std::sqrt((numLocations * PERCENTAGE_OF_SEEDING_LOCATIONS) / 4), 1);
    
    int locationX = uniqueIdx % synLocationGridWidth;
    int locationY = uniqueIdx / synLocationGridWidth;
    if ((locationX < seedSize || (synLocationGridWidth - locationX) <= seedSize)
        && (locationY < seedSize || (synLocationGridHeight - locationY) <= seedSize)) {
      isDiseaseSeeder = true;
    }
  } else {
    // For non-synthetic set just seed completely at random.
    isDiseaseSeeder = unitDistrib(*generator) < PERCENTAGE_OF_SEEDING_LOCATIONS;
  }
  
}

Location::Location(CkMigrateMessage *msg) {};

void Location::pup(PUP::er &p) {
  p | infectiousArrivals;
  p | susceptibleArrivals;
  p | interactions;
  p | locationData;
  p | isDiseaseSeeder;
  p | day;
  p | uniqueId;
  p | events;

  if (p.isUnpacking()) {
    unitDistrib = std::uniform_real_distribution<>(0.0,1.0);
  }
}

// Lets location partition refresh generator after migration, since pointers
// probably won't survive the migration
void Location::setGenerator(std::default_random_engine *generator) {
  this->generator = generator;
}

// DataInterface overrides. 
void Location::setUniqueId(int idx) {
    this->uniqueId = idx;
}

std::vector<union Data> &Location::getDataField() {
    return this->locationData;
}

// Event processing.
void Location::addEvent(Event e) {
  events.push_back(e);
}

void Location::processEvents(
  const DiseaseModel *diseaseModel,
  ContactModel *contactModel
) {
  std::vector<Event> *arrivals;
  
  std::sort(events.begin(), events.end());
  for (const Event &event: events) {
    if (diseaseModel->isSusceptible(event.personState)) {
      arrivals = &susceptibleArrivals;

    } else if (diseaseModel->isInfectious(event.personState)) {
      arrivals = &infectiousArrivals;

    // If a person can niether infect other people nor be infected themself,
    // we can just ignore their comings and goings
    } else {
      continue;
    }

    if (ARRIVAL == event.type) {
      arrivals->push_back(event);
      std::push_heap(arrivals->begin(), arrivals->end(), Event::greaterPartner);

    } else if (DEPARTURE == event.type) {
      // Remove the arrival event corresponding to this departure 
      std::pop_heap(arrivals->begin(), arrivals->end(), Event::greaterPartner);
      arrivals->pop_back();

      onDeparture(diseaseModel, contactModel, event);
    }
  }

  events.clear();
  interactions.clear();
  day++;
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
  if (!contactModel->madeContact(susceptibleEvent, infectiousEvent, *this)) {
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
    numPeoplePartitions,
    firstPersonIdx
  );

  // Randomly seed some people for infection.
  if (isDiseaseSeeder && day < DAYS_TO_SEED_INFECTION 
      && unitDistrib(*generator) < INITIAL_INFECTIOUS_PROBABILITY) {
        // Add a super contagious visit for that person.
        interactions[personIdx].emplace_back(
          std::numeric_limits<double>::max(),
          0,
          0,
          0,
          std::numeric_limits<int>::max()
        );
  }
  InteractionMessage interMsg(personIdx, interactions[personIdx]);
  if (aggregator) {
    aggregator->send(peopleArray[peoplePartitionIdx], interMsg);
  } else {
    peopleArray[peoplePartitionIdx].ReceiveInteractions(interMsg);
  }

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

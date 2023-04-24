/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "Types.h"
#include "Location.h"
#include "People.h"
#include "Event.h"
#include "Defs.h"
#include "Extern.h"
#include "DiseaseModel.h"
#include "contact_model/ContactModel.h"

#ifdef USE_HYPERCOMM
  #include "Aggregator.h"
#endif  // USE_HYPERCOMM

#include <random>
#include <vector>
#include <cmath>
#include <algorithm>

Location::Location(int numAttributes, int uniqueIdx,
    std::default_random_engine *generator, const DiseaseModel *diseaseModel) :
  unitDistrib(0, 1) {
  /*if (numAttributes != 0) {
    this->locationData.resize(numAttributes);
    }*/
  int tableSize = diseaseModel->locationTable.size();
  if (tableSize != 0) {
    this->data.resize(tableSize);
    for (int i = numAttributes; i < tableSize; i++) {
      data[i] = diseaseModel->locationTable.getDefaultValue(i);
    }
  }
  day = 0;
  this->generator = generator;

  if (interventionStategy) {
    complysWithShutdown = diseaseModel->complyingWithLockdown(generator);
  }
}

Location::Location(CkMigrateMessage *msg) {}

void Location::pup(PUP::er &p) {
  p | infectiousArrivals;
  p | susceptibleArrivals;
  p | interactions;
  p | data;
  p | day;
  p | uniqueId;
  p | events;
}

// Lets location partition refresh generator after migration, since pointers
// probably won't survive the migration
void Location::setGenerator(std::default_random_engine *generator) {
  this->generator = generator;
}

// Event processing.
void Location::addEvent(Event e) {
  events.push_back(e);
}

Counter Location::processEvents(
  const DiseaseModel *diseaseModel,
  ContactModel *contactModel
) {
  Counter numInteractions = 0;
  std::vector<Event> *arrivals;
  #if ENABLE_DEBUG >= DEBUG_VERBOSE
  Counter numPresent = 0;
  #endif

  if (!interventionStategy || !complysWithShutdown
      || diseaseModel->isLocationOpen(&data)) {
    std::sort(events.begin(), events.end());
    for (const Event &event : events) {
      if (diseaseModel->isSusceptible(event.personState)) {
        arrivals = &susceptibleArrivals;

      } else if (diseaseModel->isInfectious(event.personState)) {
        arrivals = &infectiousArrivals;

      // If a person can niether infect other people nor be infected themself,
      // we can just ignore their comings and goings
      } else {
        #if ENABLE_DEBUG >= DEBUG_VERBOSE
        if (ARRIVAL == event.type) {
          numPresent++;
        } else {
          numPresent--;
          numInteractions += numPresent;
        }
        #endif
        continue;
      }

      if (ARRIVAL == event.type) {
        arrivals->push_back(event);
        std::push_heap(arrivals->begin(), arrivals->end(), Event::greaterPartner);
        #if ENABLE_DEBUG >= DEBUG_VERBOSE
        numPresent++;
        #endif
      } else if (DEPARTURE == event.type) {
        // Remove the arrival event corresponding to this departure
        std::pop_heap(arrivals->begin(), arrivals->end(), Event::greaterPartner);
        arrivals->pop_back();
        #if ENABLE_DEBUG >= DEBUG_VERBOSE
        numPresent--;
        numInteractions += numPresent;
        #endif

        onDeparture(diseaseModel, contactModel, event);
      }
    }
  }
  events.clear();
  interactions.clear();
  day++;

  #ifdef ENABLE_DEBUG >= DEBUG_VERBOSE
  return contactModel->getContactProbability(*this) * numInteractions;
  #else
  return 0;
  #endif
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
  for (const Event &infectiousArrival : infectiousArrivals) {
    registerInteraction(
      diseaseModel,
      contactModel,
      susceptibleDeparture,
      infectiousArrival,
      // The start time is whichever arrival happened later
      std::max(
        infectiousArrival.scheduledTime,
        susceptibleDeparture.partnerTime),
      susceptibleDeparture.scheduledTime);
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
        infectiousDeparture.partnerTime),
      infectiousDeparture.scheduledTime);
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
    endTime);

  // Note that this will create a new vector if this is the first potential
  // infection for the susceptible person in question
  interactions[susceptibleEvent.personIdx].emplace_back(
    propensity,
    infectiousEvent.personIdx,
    infectiousEvent.personState,
    startTime,
    endTime);
}

// Simple helper function which send the list of interactions with the
// specified person to the appropriate People chare
inline void Location::sendInteractions(int personIdx) {
  int peoplePartitionIdx = getPartitionIndex(
    personIdx,
    numPeople,
    numPeoplePartitions,
    firstPersonIdx);

  InteractionMessage interMsg(uniqueId, personIdx, interactions[personIdx]);
  #ifdef USE_HYPERCOMM
  Aggregator* agg = aggregatorProxy.ckLocalBranch();
  if (agg->interact_aggregator) {
    agg->interact_aggregator->send(peopleArray[peoplePartitionIdx], interMsg);
  } else {
  #endif  // USE_HYPERCOMM
    peopleArray[peoplePartitionIdx].ReceiveInteractions(interMsg);
  #ifdef USE_HYPERCOMM
  }
  #endif  // USE_HYPERCOMM

  if (0 > personIdx) {
    CkAbort("   Trying to send message to person %d\n", personIdx);
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

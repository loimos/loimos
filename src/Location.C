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

#include <random>
#include <set>

std::uniform_real_distribution<> Location::unitDistrib(0,1);

void Location::addEvent(Event e) {
  events.push(e);
}

std::unordered_set<int> Location::processEvents(
  std::default_random_engine generator
) {
  std::vector<int> *people;
  Event curEvent;
  justInfected.empty();

  while (!events.empty()) {
    curEvent = events.top();
    events.pop();

    // TODO: implement a disease model to make this check properly
    if (SUSCEPTIBLE == curEvent.personState) {
      people = &susceptiblePeople;

    } else if (INFECTIOUS == curEvent.personState) {
      people = &infectiousPeople;

    } else {
      continue;
    }

    if (ARRIVAL == curEvent.type) {
      people->push_back(curEvent.personIdx);

    } else if (DEPARTURE == curEvent.type) {
      people->erase(
        std::remove(
          people->begin(),
          people->end(),
          curEvent.personIdx
        ), people->end()
      );

      onDeparture(curEvent, generator);
    }
  }

  return justInfected;
}

inline void Location::onDeparture(
  Event event,
  std::default_random_engine generator
) {
  if (SUSCEPTIBLE == event.personState) {
    onSusceptibleDeparture(event.personIdx, generator);

  } else if (INFECTIOUS == event.personState) {
    onInfectiousDeparture(event.personIdx, generator);

  } 
}

// The infection probability amy eventually depend on traits of the
// infectious or susceptible person, which is why we need the personIdx
void Location::onInfectiousDeparture(
  int infectiousIdx,
  std::default_random_engine generator
) {
  
  for (int susceptibleIdx: susceptiblePeople) {
    if (unitDistrib(generator) < INFECTION_PROBABILITY) { 
      justInfected.insert(susceptibleIdx);
    }
  } 
}

void Location::onSusceptibleDeparture(
  int susceptibleIdx,
  std::default_random_engine generator
) {
  double probNotInfected = 1.0;
  for (int infectiousIdx: infectiousPeople) {
    probNotInfected *= 1.0 - INFECTION_PROBABILITY;
  }
  
  // We want the probability of infection, so we need to 
  // invert probNotInfected
  if (unitDistrib(generator) > probNotInfected) {
    justInfected.insert(susceptibleIdx);
  }
}

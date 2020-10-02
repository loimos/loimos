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
  
  //TODO: implement location infectivity
  //if (diseaseModel->isInfectious(personState))
  //  locationState[localLocIdx] = INFECTIOUS;
}

std::unordered_set<int> Location::processEvents(
  std::default_random_engine generator
) {
  std::vector<int> people;
  Event curEvent;
  justInfected.empty();

  while (!events.empty()) {
    curEvent = events.top();
    events.pop();

    // TODO: implement a disease model to make this check properly
    if (SUSCEPTIBLE == curEvent.personState) {
      people = susceptiblePeople;
    
    } else if (INFECTIOUS == curEvent.personState) {
      people = infectiousPeople;

    } else {
      continue;
    }

    if (ARRIVAL == curEvent.type) {
      people.push_back(curEvent.personIdx);

    } else if (DEPARTURE == curEvent.type) {
      people.erase(
        std::remove(
          people.begin(),
          people.end(),
          curEvent.personIdx
        ), people.end()
      );

      if (SUSCEPTIBLE == curEvent.personState) {
        onInfectiousDeparture(curEvent.personIdx, generator);

      } else if (INFECTIOUS == curEvent.personState) {
        onSuspectibleDeparture(curEvent.personIdx, generator);

      } 
    }
  }

  return justInfected;
}

// The infection probability amy eventually depend on traits of the
// infectious or susceptible person, which is why we need the personIdx
void Location::onInfectiousDeparture(
  int personIdx,
  std::default_random_engine generator
) { 
  for (int otherIdx: susceptiblePeople) {
    if (unitDistrib(generator) < INFECTION_PROBABILITY) 
      justInfected.insert(personIdx);
  } 
}

void Location::onSuspectibleDeparture(
  int personIdx,
  std::default_random_engine generator
) {
  double probNotInfected = 1.0;
  for (int otherIdx: infectiousPeople) {
    probNotInfected *= 1.0 - INFECTION_PROBABILITY;
  }

  if (unitDistrib(generator) < probNotInfected)
    justInfected.insert(personIdx);
}

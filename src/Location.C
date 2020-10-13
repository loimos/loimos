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
#include <cmath>

std::uniform_real_distribution<> Location::unitDistrib(0,1);

void Location::addEvent(Event e) {
  events.push(e);
}

std::unordered_set<int> Location::processEvents(
  std::default_random_engine generator
) {
  std::vector<Event> *arrivals;
  Event curEvent;
  justInfected.empty();

  while (!events.empty()) {
    curEvent = events.top();
    events.pop();

    // TODO: implement a disease model to make this check properly
    if (SUSCEPTIBLE == curEvent.personState) {
      arrivals = &susceptibleArrivals;

    } else if (INFECTIOUS == curEvent.personState) {
      arrivals = &infectiousArrivals;

    } else {
      continue;
    }

    if (ARRIVAL == curEvent.type) {
      arrivals->push_back(curEvent);

    } else if (DEPARTURE == curEvent.type) {
      int curIdx = curEvent.personIdx;
      arrivals->erase(
        std::remove_if(
          arrivals->begin(),
          arrivals->end(),
          [curIdx](Event e) {
            return e.personIdx == curIdx;
          }
        ), arrivals->end()
      );
      /*
      std::erase_if(arrivals, [](Event e) {
         return e.personIdx == curEvent.personIdx;
      });
      */

      onDeparture(curEvent, generator);
    }
  }

  return justInfected;
}

inline void Location::onDeparture(
  Event departure,
  std::default_random_engine generator
) {
  if (SUSCEPTIBLE == departure.personState) {
    onSusceptibleDeparture(departure, generator);

  } else if (INFECTIOUS == departure.personState) {
    onInfectiousDeparture(departure, generator);
  } 
}

// Put this in the disease model, once it exists
// Also, this currently assumes the infection probability is constant,
// so if we start having that depend on people's characteristics, this
// will need to change
double getLogProbNotInfected(Event arrival, Event departure) {
  double baseProb = 1.0 - INFECTION_PROBABILITY;
  return log(baseProb) * (departure.time - arrival.time);
}

// The infection probability amy eventually depend on traits of the
// infectious or susceptible person, which is why we need the personIdx
void Location::onInfectiousDeparture(
  Event d,
  std::default_random_engine generator
) {
  
  for (Event a : susceptibleArrivals) {
    // We want the probability of infection, so we need to 
    // invert probNotInfected
    if (unitDistrib(generator) > exp(getLogProbNotInfected(a, d))) { 
      justInfected.insert(a.personIdx);
    }
  } 
}

void Location::onSusceptibleDeparture(
  Event d,
  std::default_random_engine generator
) {
  double logProbNotInfected = 0.0;
  for (Event a: infectiousArrivals) {
    logProbNotInfected += getLogProbNotInfected(a, d);
  }
  
  // We want the probability of infection, so we need to 
  // invert probNotInfected
  if (unitDistrib(generator) > exp(logProbNotInfected)) {
    justInfected.insert(d.personIdx);
  }
}

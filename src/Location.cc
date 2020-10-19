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
#include <stdio.h>

std::uniform_real_distribution<> Location::unitDistrib(0,1);

void Location::addEvent(Event e) {
  events.push(e);
  
  //TODO: implement location infectivity
  //if (diseaseModel->isInfectious(personState))
  //  locationState[localLocIdx] = INFECTIOUS;
}

std::unordered_set<int> Location::processEvents(
  std::default_random_engine *generator,
  DiseaseModel *diseaseModel
) {
  std::vector<Event> *arrivals;
  Event curEvent;
  justInfected.empty();

  while (!events.empty()) {
    curEvent = events.top();
    events.pop();

    if (diseaseModel->isSusceptible(curEvent.personState)) {
      /*
      printf(
        "Handling susceptible arrival %s\n\r",
        diseaseModel->getStateLabel(curEvent.personState)
      );
      */
      arrivals = &susceptibleArrivals;

    } else if (diseaseModel->isInfectious(curEvent.personState)) {
      /*
      printf(
        "Handling infectious arrival %s\n\r",
        diseaseModel->getStateLabel(curEvent.personState)
      );
      */
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

      onDeparture(generator, diseaseModel, curEvent);
    }
  }

  return justInfected;
}

inline void Location::onDeparture(
  std::default_random_engine *generator,
  DiseaseModel *diseaseModel,
  Event departure
) {
  if (diseaseModel->isSusceptible(departure.personState)) {
    onSusceptibleDeparture(generator, diseaseModel, departure);

  } else if (diseaseModel->isInfectious(departure.personState)) {
    onInfectiousDeparture(generator, diseaseModel, departure);
  } 
}

void Location::onSusceptibleDeparture(
  std::default_random_engine *generator,
  DiseaseModel *diseaseModel,
  Event susceptibleDeparture
) {
  double logProbNotInfected = 0.0;
  for (Event infectiousArrival: infectiousArrivals) {
    logProbNotInfected += diseaseModel->getLogProbNotInfected(
      susceptibleDeparture, infectiousArrival
    );
  }

  // We want the probability of infection, so we need to 
  // invert probNotInfected
  double prob = exp(logProbNotInfected);
  double roll = unitDistrib(*generator);
  /*
  printf(
    "Infection prob: %f (from %d contacts, rolled %f)\n\r",
    1.0 - prob,
    (int) infectiousArrivals.size(),
    roll
  );
  */
  if (roll > prob) {
    justInfected.insert(susceptibleDeparture.personIdx);
  }
}

void Location::onInfectiousDeparture(
  std::default_random_engine *generator,
  DiseaseModel *diseaseModel,
  Event infectiousDeparture
) {
  
  for (Event susceptibleArrival : susceptibleArrivals) {
    // We want the probability of infection, so we need to 
    // invert probNotInfected
    double prob = exp(diseaseModel->getLogProbNotInfected(
      susceptibleArrival, infectiousDeparture
    ));

    double roll = unitDistrib(*generator);
    //printf("Infection prob: %f (rolled %f)\n\r", 1.0 - prob, roll);
    if (roll > prob) { 
      justInfected.insert(susceptibleArrival.personIdx);
    }
  } 
}

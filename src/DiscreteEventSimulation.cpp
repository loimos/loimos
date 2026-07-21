/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "DiscreteEventSimulation.h"
#include "Types.h"
#include "Location.h"
#include "DiseaseModel.h"
#include "Event.h"
#include "Defs.h"
#include "Scenario.h"
#include "Interaction.h"
#include "contact_model/ContactModel.h"

#include <vector>
#include <algorithm>
#include <unordered_map>

void queueVisitsImpl(std::vector<Location> *locations,
   const Scenario* scenario, int day,
   const std::unordered_map<Id, PersonState> &visitorStates) {
  for (Location &location : *locations) {
    const std::vector<VisitMessage> &visits =
      location.visitsByDay[day % scenario->numDaysWithDistinctVisits];
    for (const VisitMessage &visit : visits) {
      if (!visit.isActive()) {
        continue;
      }

      const PersonState &state = visitorStates.at(visit.personIdx);
      Event arrival { ARRIVAL, visit.personIdx, state.state,
        state.transmissionModifier, visit.visitStart };
      Event departure { DEPARTURE, visit.personIdx, state.state,
        state.transmissionModifier, visit.visitEnd };
      Event::pair(&arrival, &departure);

      location.addEvent(arrival);
      location.addEvent(departure);

#ifdef ENABLE_SC
      bool isInfectious = scenario->diseaseModel->isInfectious(state.state);
      if (!location.anyInfectious && isInfectious) {
        location.anyInfectious = true;
      }
#endif
    }
  }
}

// Runs through all of the current events and return the indices of
// any people who have been infected
Counter processEvents(Location *loc, Scenario *scenario,
  std::ofstream *interactionsFile, int thisIndex,
  std::unordered_map<Id, std::vector<Interaction>> *interactions) {
  std::vector<Event> *arrivals;
  std::vector<Event> infectiousArrivals;
  std::vector<Event> susceptibleArrivals;
#if ENABLE_DEBUG >= DEBUG_VERBOSE
  Counter numInteractions = 0;
  Counter numPresent = 0;
  Counter numVisits = loc->events.size() / 2;
  Counter duration = 0;
  double startTime = CkWallTimer();
#elif ENABLE_SC
  if (!loc->anyInfectious) {
    loc->reset();
    return 0;
  }
#endif

  std::sort(loc->events.begin(), loc->events.end());
  for (const Event &event : loc->events) {
#if ENABLE_DEBUG >= DEBUG_VERBOSE
    if (ARRIVAL == event.type) {
      numPresent++;
    } else {
      numPresent--;
      numInteractions += numPresent;
    }
#endif

    DiseaseModel *diseaseModel = scenario->diseaseModel;
    if (diseaseModel->isSusceptible(event.personState)) {
      arrivals = &susceptibleArrivals;

    } else if (diseaseModel->isInfectious(event.personState)) {
      arrivals = &infectiousArrivals;

    // If a person can neither infect other people nor be infected themself,
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

#if OUTPUT_FLAGS & OUTPUT_OVERLAPS
      saveInteractions(*loc, event, interactionsFile, susceptibleArrivals,
        infectiousArrivals);
#endif

      onDeparture(loc, scenario, event, susceptibleArrivals, infectiousArrivals,
        interactions, thisIndex);
    }
  }
  loc->reset();

#if ENABLE_DEBUG >= DEBUG_VERBOSE
  double p = scenario->contactModel->getContactProbability(*loc);
  Counter total = static_cast<Counter>(p * numInteractions);

#if ENABLE_DEBUG == DEBUG_LOCATION_SUMMARY
  if (0 != numInteractions && -1 != maxSimVisitsIdx) {
    double elapsedTime = CkWallTimer() - startTime;
    CkPrintf("      %d,%d,%f," COUNTER_PRINT_TYPE "," COUNTER_PRINT_TYPE ","
        COUNTER_PRINT_TYPE",%f\n",
        loc->getUniqueId(), loc->getValue(maxSimVisitsIdx).int32_val,
        p, numInteractions, total, numVisits, elapsedTime);
  }
#endif  // DEBUG_LOCATION_SUMMARY
  return total;
#else
  return 0;
#endif  // DEBUG_VERBOSE
}

// Writes a line per overlap containing information about
// the overlapping people. For debugging/instrumentation
// purposes.
#if OUTPUT_FLAGS & OUTPUT_OVERLAPS
Counter saveInteractions(const Location &loc,
    const Event &departure, std::ofstream *out,
    const std::vector<Event> &susceptibleArrivals,
    const std::vector<Event> &infectiousArrivals) {
  Counter duration = 0;
  Time end = departure.scheduledTime;
  for (const Event &a : susceptibleArrivals) {
    if (Event::overlap(a, departure)) {
      if (NULL != out) {
        *out << loc.getUniqueId() << "," << departure.personIdx << ","
          << departure.partnerTime << ","  << departure.scheduledTime << ","
          << a.personIdx << "," << a.scheduledTime << "," << a.partnerTime
          << std::endl;
      }

      Time start = std::max(a.scheduledTime, departure.partnerTime);
      duration += end - start;
    }
  }
  for (const Event &a : infectiousArrivals) {
    if (Event::overlap(a, departure)) {
      if (NULL != out) {
        *out << loc.getUniqueId() << "," << departure.personIdx << ","
          << departure.partnerTime << ","  << departure.scheduledTime << ","
          << a.personIdx << "," << a.scheduledTime << "," << a.partnerTime
          << std::endl;
      }

      Time start = std::max(a.scheduledTime, departure.partnerTime);
      duration += end - start;
    }
  }
  return duration;
}
#endif  // OUTPUT_OVERLAPS

// Simple dispatch to the susceptible/infectious departure handlers
inline void onDeparture(Location *loc, Scenario *scenario, const Event& departure,
  const std::vector<Event> &susceptibleArrivals,
  const std::vector<Event> &infectiousArrivals,
  std::unordered_map<Id, std::vector<Interaction>> *interactions, int thisIndex) {
  DiseaseModel *diseaseModel = scenario->diseaseModel;

  if (diseaseModel->isSusceptible(departure.personState)) {
    onSusceptibleDeparture(loc, scenario, departure, infectiousArrivals, interactions,
      thisIndex);

  } else if (diseaseModel->isInfectious(departure.personState)) {
    onInfectiousDeparture(loc, scenario, departure, susceptibleArrivals, interactions);
  }
}

// Handles a susceptible person's departure, registering any interactions
// with infectious people.
void onSusceptibleDeparture(Location *loc, Scenario *scenario,
    const Event& susceptibleDeparture, const std::vector<Event> &infectiousArrivals,
    std::unordered_map<Id, std::vector<Interaction>> *interactions, int thisIndex) {
  // Each infectious person at this location might have infected this
  // susceptible person
  for (const Event &infectiousArrival : infectiousArrivals) {
    registerInteraction(loc, scenario, susceptibleDeparture, infectiousArrival,
      // The start time is whichever arrival happened later
      std::max(infectiousArrival.scheduledTime,
        susceptibleDeparture.partnerTime),
        susceptibleDeparture.scheduledTime, interactions);
  }
}

// Handles an infectious person's departure, registering any interactions
// with susceptible people.
void onInfectiousDeparture(Location *loc, Scenario *scenario,
    const Event& infectiousDeparture, const std::vector<Event> &susceptibleArrivals,
    std::unordered_map<Id, std::vector<Interaction>> *interactions) {
  // Each susceptible person at this location might have been infected by this
  // infectious person
  for (const Event &susceptibleArrival : susceptibleArrivals) {
    registerInteraction(loc, scenario, susceptibleArrival, infectiousDeparture,
      // The start time is whichever arrival happened later
      std::max(susceptibleArrival.scheduledTime,
        infectiousDeparture.partnerTime),
      infectiousDeparture.scheduledTime, interactions);
  }
}

// Helper function which packages all the necessary information about
// an interaction between a susceptible person and an infectious person
// and adds it to the appropriate list for the susceptible person
inline void registerInteraction(Location *loc, Scenario *scenario,
    const Event &susceptibleEvent, const Event &infectiousEvent, Time startTime,
    Time endTime, std::unordered_map<Id, std::vector<Interaction>> *interactions) {
  if (!scenario->contactModel->madeContact(susceptibleEvent, infectiousEvent, loc)) {
    return;
  }

  double propensity = scenario->diseaseModel->getPropensity(
    susceptibleEvent.personState, infectiousEvent.personState, startTime, endTime,
    susceptibleEvent.transmissionModifier, infectiousEvent.transmissionModifier);

  // Note that this will create a new vector if this is the first potential infection
  // for the susceptible person in question
  Interaction inter { propensity, infectiousEvent.personIdx,
    infectiousEvent.personState, startTime, endTime };
  (*interactions)[susceptibleEvent.personIdx].emplace_back(inter);
}

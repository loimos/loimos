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
#include "Extern.h"

#include <vector>
// Forward declarations
Counter processEvents(Location *loc, Scenario *scenario);

#if OUTPUT_FLAGS & OUTPUT_OVERLAPS
Counter saveInteractions(const Location &loc,
    const Event &departure, std::ofstream *out, 
    std::vector<Event> &susceptibleArrivals, 
    std::vector<Event> &infectiousArrivals);
#endif  // OUTPUT_OVERLAPS

inline void onDeparture(Location *loc, Scenario *scenario, const Event& departure, 
    const std::vector<Event> &susceptibleArrivals, 
    const std::vector<Event> &infectiousArrivals, 
    std::unordered_map<Id, std::vector<Interaction>> *interactions);

void onSusceptibleDeparture(Location *loc, Scenario *scenario,
    const Event& susceptibleDeparture, 
    const std::vector<Event> &infectiousArrivals, 
    std::unordered_map<Id, std::vector<Interaction>> *interactions);

void onInfectiousDeparture(Location *loc, Scenario *scenario,
    const Event& infectiousDeparture, 
    const std::vector<Event> &susceptibleArrivals, 
    std::unordered_map<Id, std::vector<Interaction>> *interactions);

inline void registerInteraction(Location *loc, Scenario *scenario,
    const Event &susceptibleEvent, const Event &infectiousEvent,
    Time startTime, Time endTime, 
    std::unordered_map<Id, std::vector<Interaction>> *interactions);

inline void sendInteractions(Location *loc, Scenario *scenario, 
    Id personIdx, std::unordered_map<Id, std::vector<Interaction>> *interactions);

void ComputeInteractions(std::vector<Location> *locations, Scenario *scenario, int day) {
  Counter numVisits = 0;
  Counter numInteractions = 0;
  for (Location &loc : *locations) {
    Counter locVisits = loc.events.size() / 2;
    numVisits += locVisits;

    Counter locInters = processEvents(&loc, scenario);
    numInteractions += locInters;
  }
#if ENABLE_DEBUG >= DEBUG_VERBOSE
  CkCallback cb(CkReductionTarget(Main, ReceiveInteractionsCount), mainProxy);
  contribute(sizeof(Counter), &numInteractions,
      CONCAT(CkReduction::sum_, COUNTER_REDUCTION_TYPE), cb);
#endif

#if ENABLE_DEBUG >= DEBUG_PER_CHARE
  if (0 == day) {
    CkPrintf("    Process %d, thread %d: " COUNTER_PRINT_TYPE " visits, "
        COUNTER_PRINT_TYPE" interactions, %lu locations\n",
        CkMyNode(), CkMyPe(), numVisits, numInteractions, locations.size());
  }
#endif

}

Counter processEvents(Location *loc, Scenario *scenario) {
  std::vector<Event> *arrivals;
  std::vector<Event> infectiousArrivals;
  std::vector<Event> susceptibleArrivals;
  std::unordered_map<Id, std::vector<Interaction>> interactions;
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
      saveInteractions(*loc, event, interactionsFile, susceptibleArrivals, infectiousArrivals);
#endif

      onDeparture(loc, scenario, event, susceptibleArrivals, infectiousArrivals, &interactions);
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

#if OUTPUT_FLAGS & OUTPUT_OVERLAPS
Counter saveInteractions(const Location &loc,
    const Event &departure, std::ofstream *out, const std::vector<Event> &susceptibleArrivals, const std::vector<Event> &infectiousArrivals) {
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

// Simple dispatch to the susceptible/infectious depature handlers
inline void onDeparture(Location *loc, Scenario *scenario, const Event& departure, const std::vector<Event> &susceptibleArrivals, const std::vector<Event> &infectiousArrivals, std::unordered_map<Id, std::vector<Interaction>> *interactions) {
  DiseaseModel *diseaseModel = scenario->diseaseModel;

  if (diseaseModel->isSusceptible(departure.personState)) {
    onSusceptibleDeparture(loc, scenario, departure, infectiousArrivals, interactions);

  } else if (diseaseModel->isInfectious(departure.personState)) {
    onInfectiousDeparture(loc, scenario, departure, susceptibleArrivals, interactions);
  }
}

void onSusceptibleDeparture(Location *loc, Scenario *scenario,
    const Event& susceptibleDeparture, const std::vector<Event> &infectiousArrivals, std::unordered_map<Id, std::vector<Interaction>> *interactions) {
  // Each infectious person at this location might have infected this
  // susceptible person
  for (const Event &infectiousArrival : infectiousArrivals) {
    registerInteraction(loc, scenario, susceptibleDeparture, infectiousArrival,
      // The start time is whichever arrival happened later
      std::max(infectiousArrival.scheduledTime,
        susceptibleDeparture.partnerTime),
        susceptibleDeparture.scheduledTime, interactions);
  }

  sendInteractions(loc, scenario, susceptibleDeparture.personIdx, interactions);
}

void onInfectiousDeparture(Location *loc, Scenario *scenario,
    const Event& infectiousDeparture, const std::vector<Event> &susceptibleArrivals, std::unordered_map<Id, std::vector<Interaction>> *interactions) {
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

inline void registerInteraction(Location *loc, Scenario *scenario,
    const Event &susceptibleEvent, const Event &infectiousEvent,
    Time startTime, Time endTime, std::unordered_map<Id, std::vector<Interaction>> *interactions) {
  if (!scenario->contactModel->madeContact(susceptibleEvent, infectiousEvent, loc)) {
    return;
  }

  double propensity = scenario->diseaseModel->getPropensity(
    susceptibleEvent.personState, infectiousEvent.personState, startTime, endTime,
    susceptibleEvent.transmissionModifier, infectiousEvent.transmissionModifier);

  // Note that this will create a new vector if this is the first potential
  // infection for the susceptible person in question
  Interaction inter { propensity, infectiousEvent.personIdx,
    infectiousEvent.personState, startTime, endTime };
  (*interactions)[susceptibleEvent.personIdx].emplace_back(inter);
}

// Simple helper function which send the list of interactions with the
// specified person to the appropriate People chare
inline void sendInteractions(Location *loc, Scenario *scenario, 
    Id personIdx, std::unordered_map<Id, std::vector<Interaction>> *interactions) {
  Partitioner *partitioner = scenario->partitioner;
  PartitionId personPartition = partitioner->getPersonPartitionIndex(personIdx);
#ifdef ENABLE_DEBUG
  if (outOfBounds(0, partitioner->getNumPersonPartitions(), personPartition)) {
    CkAbort("Error on chare %d: sending exposures at "
      ID_PRINT_TYPE" to person " ID_PRINT_TYPE " on chare "
      PARTITION_ID_PRINT_TYPE" outside of valid range [0, "
      PARTITION_ID_PRINT_TYPE")\n", thisIndex, loc->getUniqueId(),
      personIdx, personPartition, partitioner->getNumPersonPartitions());
  }
#endif

  InteractionMessage interMsg(loc->getUniqueId(), personIdx,
      (*interactions)[personIdx]);
#ifdef USE_HYPERCOMM
  Aggregator *agg = aggregatorProxy.ckLocalBranch();
  if (agg->interact_aggregator) {
    agg->interact_aggregator->send(peopleArray[personPartition], interMsg);
    continue;
  }
#endif  // USE_HYPERCOMM

  peopleArray[personPartition].ReceiveInteractions(interMsg);

  // Free up space where we were storing interactions data. This also prevents
  // interactions from being sent multiple times if this person has multiple
  // visits to this location
  interactions->erase(personIdx);
}

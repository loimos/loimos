/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef DISCRETEEVENTSIMULATION_H_
#define DISCRETEEVENTSIMULATION_H_

#include "Location.h"
#include "Scenario.h"
#include "Event.h"
#include "Interaction.h"

#include <vector>
#include <unordered_map>

// Processes all of the current events and returns indices of
// people who have been infected.
Counter processEvents(Location *loc, Scenario *scenario,
    std::ofstream *interactionsFile, int thisIndex, std::unordered_map<Id, 
    std::vector<Interaction>> *interactions);

// Debugging/Instrumentation
// Writes visit overlaps to an output stream
#if OUTPUT_FLAGS & OUTPUT_OVERLAPS
Counter saveInteractions(const Location &loc,
    const Event &departure, std::ofstream *out,
    const std::vector<Event> &susceptibleArrivals,
    const std::vector<Event> &infectiousArrivals);
#endif  // OUTPUT_OVERLAPS

// Helper functions that handle when a person leaves a location.
// onDeparture branches to one of the other two.
inline void onDeparture(Location *loc, Scenario *scenario, const Event& departure,
    const std::vector<Event> &susceptibleArrivals,
    const std::vector<Event> &infectiousArrivals,
    std::unordered_map<Id, std::vector<Interaction>> *interactions, int thisIndex);
void onSusceptibleDeparture(Location *loc, Scenario *scenario,
    const Event& susceptibleDeparture,
    const std::vector<Event> &infectiousArrivals,
    std::unordered_map<Id, std::vector<Interaction>> *interactions, int thisIndex);
void onInfectiousDeparture(Location *loc, Scenario *scenario,
    const Event& infectiousDeparture,
    const std::vector<Event> &susceptibleArrivals,
    std::unordered_map<Id, std::vector<Interaction>> *interactions);

// Registers information about an interaction between a susceptible
// and infectious person.
inline void registerInteraction(Location *loc, Scenario *scenario,
    const Event &susceptibleEvent, const Event &infectiousEvent,
    Time startTime, Time endTime,
    std::unordered_map<Id, std::vector<Interaction>> *interactions);

#endif  // DISCRETEEVENTSIMULATION_H_

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

// Runs through all of the current events and return the indices of
// any people who have been infected
Counter processEvents(Location *loc, Scenario *scenario,
    std::ofstream *interactionsFile, int thisIndex);

#if OUTPUT_FLAGS & OUTPUT_OVERLAPS
Counter saveInteractions(const Location &loc,
    const Event &departure, std::ofstream *out,
    const std::vector<Event> &susceptibleArrivals,
    const std::vector<Event> &infectiousArrivals);
#endif  // OUTPUT_OVERLAPS

// Helper functions to handle when a person leaves a location
// onDeparture branches to one of the two other functions
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

// Helper function which packages all the necessary information about
// an interaction between a susceptible person and an infectious person
// and add it to the approriate list for the susceptible person
inline void registerInteraction(Location *loc, Scenario *scenario,
    const Event &susceptibleEvent, const Event &infectiousEvent,
    Time startTime, Time endTime,
    std::unordered_map<Id, std::vector<Interaction>> *interactions);

// Simple helper function which send the list of interactions with the
// specified person to the appropriate People chare
inline void sendInteractions(Location *loc, Scenario *scenario,
    Id personIdx, std::unordered_map<Id, std::vector<Interaction>> *interactions,
    int thisIndex);

#endif  // DISCRETEEVENTSIMULATION_H_

/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __LOCATION_H__
#define __LOCATION_H__

#include "Event.h"

#include <queue>
#include <vector>
#include <functional>
#include <random>
#include <set>

// Represents a single location where people can interact
// Not to be confused with Locations, which represents a group of
// intances of this class
class Location {
  private:
    // Represents all of the arrivals and departures of people
    // from this location on a given day
    std::priority_queue<Event, std::vector<Event>, std::greater<Event> > events;
    // Each int in one of these containers represents the index of
    // a person at this location
    std::vector<int> infectiousPeople;
    std::vector<int> susceptiblePeople;
    std::unordered_set<int> justInfected;

    // Helper functions to handle when a person leaves this location
    void onInfectiousDeparture(
      int personIdx,
      std::default_random_engine generator
    );
    void onSuspectibleDeparture(
      int personIdx,
      std::default_random_engine generator
    );
  public:
    // just use default constructors
   
    // This distribution shoul always be the same - not sure how well
    // static variables work with Charm++, so this may need to be put
    // on the stack somehwer later on
    static std::uniform_real_distribution<> unitDistrib;
    // Adds an event represnting a person either arriving or departing
    // from this location
    void addEvent(Event e);
    // Runs through all of the current events and return the indices of
    // any people who have been infected
    std::unordered_set<int> processEvents(
      std::default_random_engine generator
    );
};
  
#endif // __LOCATION_H__

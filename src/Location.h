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
  public:
    // just use default constructors
    
    void addEvent(Event e);
    void processEvents();
};
  
#endif // __LOCATION_H__

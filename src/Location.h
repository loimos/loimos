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
<<<<<<< HEAD
#include <random>
#include <set>
=======
>>>>>>> cherry-pick of actually adding in Location class and Event struct

// Represents a single location where people can interact
// Not to be confused with Locations, which represents a group of
// intances of this class
class Location {
  private:
    // Represents all of the arrivals and departures of people
    // from this location on a given day
    std::priority_queue<Event, std::vector<Event>, std::greater<Event> > events;
<<<<<<< HEAD
    // Each Event one of these containers is the arrival event for a
    // a person currently at this location
    std::vector<Event> infectiousArrivals;
    std::vector<Event> susceptibleArrivals;
    std::unordered_set<int> justInfected;

    // Helper functions to handle when a person leaves this location
    // onDeparture branches to one of the two other functions
    inline void onDeparture(
      Event departure,
      std::default_random_engine generator
    );
    void onInfectiousDeparture(
      Event departure,
      std::default_random_engine generator
    );
    void onSusceptibleDeparture(
      Event departure,
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
=======
    // Each int in one of these containers represents the index of
    // a person at this location
    std::vector<int> infectiousPeople;
    std::vector<int> susceptiblePeople;
  public:
    // just use default constructors
    
    void addEvent(Event e);
    void processEvents();
>>>>>>> cherry-pick of actually adding in Location class and Event struct
};
  
#endif // __LOCATION_H__

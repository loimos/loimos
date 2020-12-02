/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __LOCATION_H__
#define __LOCATION_H__

#include "Event.h"
#include "DiseaseModel.h"

#include <queue>
#include <vector>
#include <functional>
#include <random>
#include <set>
#include <unordered_map>

// Simple package to hold data on an interfaction between with a susceptible
// person which could lead to an infection
struct PotentialInfection {
  // Describes the chance of this interaction resulting in an infection
  double propensity;
  // Data on the person who could potentially infect the susceptible person in
  // question
  int infectorId;
  int infectorState;
  // The state the susceptible person will transition to if this results in
  // a infection
  // TODO: figure out how to extract these from an infectious/susceptible
  // state pair
  //int targetState; 
  // We need to know when the interaction occured so that, if this interaction
  // does in fact result in an infection, we can determine precisely when it
  // occured
  int startTime;
  int endTime;
};

// Represents a single location where people can interact
// Not to be confused with Locations, which represents a group of
// intances of this class
class Location {
  private:
    // Represents all of the arrivals and departures of people
    // from this location on a given day
    std::priority_queue<Event, std::vector<Event>, std::greater<Event> > events;
    // Each Event in one of these containers is the arrival event for a
    // a person currently at this location
    std::vector<Event> infectiousArrivals;
    std::vector<Event> susceptibleArrivals;
    
    // Maps each susceptible person's id to a list of interactions with people
    // who could have infected them
    std::unordered_map<int, std::vector<PotentialInfection> > potentialInfections;

    // Helper functions to handle when a person leaves this location
    // onDeparture branches to one of the two other functions
    inline void onDeparture(
      const DiseaseModel *diseaseModel,
      const Event& departure
    );
    void onSusceptibleDeparture(
      const DiseaseModel *diseaseModel,
      const Event& departure
    );
    void onInfectiousDeparture(
      const DiseaseModel *diseaseModel,
      const Event& departure
    );

    // Helper function which packages all the neccessary information about
    // a potential infection and adds it the the list of potential infections
    // for the susceptible person in question
    inline void registerPotentialInfection(
      const DiseaseModel *diseaseModel,
      Event susceptibleEvent,
      Event infectiousEvent,
      int startTime,
      int endTime
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
    void processEvents(const DiseaseModel *diseaseModel);
};
  
#endif // __LOCATION_H__

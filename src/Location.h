/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __LOCATION_H__
#define __LOCATION_H__

// Foreward declaration to help with includes
class Location;

#include "Event.h"
#include "Interaction.h"
#include "DiseaseModel.h"
#include "contact_model/ContactModel.h"
#include "readers/DataInterface.h"

#include <queue>
#include <vector>
#include <functional>
#include <random>
#include <set>
#include <unordered_map>

// Represents a single location where people can interact
// Not to be confused with Locations, which represents a group of
// intances of this class
class Location : public DataInterface {
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
    std::unordered_map<int, std::vector<Interaction> > interactions;

    // Helper functions to handle when a person leaves this location
    // onDeparture branches to one of the two other functions
    inline void onDeparture(
      const DiseaseModel *diseaseModel,
      ContactModel *contactModel,
      const Event& departure
    );
    void onSusceptibleDeparture(
      const DiseaseModel *diseaseModel,
      ContactModel *contactModel,
      const Event& departure
    );
    void onInfectiousDeparture(
      const DiseaseModel *diseaseModel,
      ContactModel *contactModel,
      const Event& departure
    );

    // Helper function which packages all the neccessary information about
    // an interaction between a susceptible person and an infectious person
    // and add it to the approriate list for the susceptible person
    inline void registerInteraction(
      const DiseaseModel *diseaseModel,
      ContactModel *contactModel,
      const Event &susceptibleEvent,
      const Event &infectiousEvent,
      int startTime,
      int endTime
    );
    // Simple helper function which send the list of interactions with the
    // specified person to the appropriate People chare
    inline void sendInteractions(int personIdx);

    // Various attributes of the location.
    std::vector<union Data> locationData;

  public:
    // Provide default constructor operations.
    Location(int numAttributes);
    Location(const Location&) = default;
    Location(Location&&) = default;
    ~Location() = default;
    // Default assignment operators.
    Location& operator=(const Location&) = default;
    Location& operator=(Location&&) = default;

    // Override abstract DataInterface getters and setters.
    void setUniqueId(int idx);
    std::vector<union Data> &getDataField();
    int uniqueId;
    
    // This distribution shoul always be the same - not sure how well
    // static variables work with Charm++, so this may need to be put
    // on the stack somehwer later on
    static std::uniform_real_distribution<> unitDistrib;
    
    // Adds an event represnting a person either arriving or departing
    // from this location
    void addEvent(Event e);
    
    // Runs through all of the current events and return the indices of
    // any people who have been infected
    void processEvents(
      const DiseaseModel *diseaseModel,
      ContactModel *contactModel
    );
};
  
#endif // __LOCATION_H__

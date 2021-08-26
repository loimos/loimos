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

#include <vector>
#include <functional>
#include <random>
#include <set>
#include <unordered_map>

#include <hypercomm/routing.hpp>
#include <hypercomm/aggregation.hpp>

// Represents a single location where people can interact
// Not to be confused with Locations, which represents a group of
// intances of this class
class Location : public DataInterface {
  private:

    // For random generation.
    std::uniform_real_distribution<> unitDistrib;
    std::default_random_engine *generator;

    // Each Event in one of these containers is the arrival event for a
    // a person currently at this location
    std::vector<Event> infectiousArrivals;
    std::vector<Event> susceptibleArrivals;
    
    // Maps each susceptible person's id to a list of interactions with people
    // who could have infected them
    std::unordered_map<int, std::vector<Interaction> > interactions;

    // Various attributes of the location.
    std::vector<union Data> locationData;

    // If this location should be an infection seeding location.
    bool isDiseaseSeeder;
    int day;
   
    // For DataInterface
    int uniqueId;
    
    // Hypercomm aggregator
    using aggregator_t = aggregation::array_aggregator<aggregation::direct_buffer, aggregation::routing::direct, InteractionMessage>;
    std::shared_ptr<aggregator_t> aggregator;

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
  
  public:
    // Represents all of the arrivals and departures of people
    // from this location on a given day
    std::vector<Event> events;
    
    // This distribution should always be the same - not sure how well
    // static variables work with Charm++, so this may need to be put
    // on the stack somewhere later on
    // static std::uniform_real_distribution<> unitDistrib;
    
    // Provide default constructor operations.
    Location() = default;
    Location(CkMigrateMessage *msg);
    Location(int numAttributes, int uniqueIdx, std::default_random_engine *generator);
    Location(const Location&) = default;
    Location(Location&&) = default;
    ~Location() = default;
    // Default assignment operators.
    Location& operator=(const Location&) = default;
    Location& operator=(Location&&) = default;
   
    // Lets us migrate these objects 
    void pup(PUP::er &p);
    void setGenerator(std::default_random_engine *generator);

    // Override abstract DataInterface getters and setters.
    void setUniqueId(int idx);
    std::vector<union Data> &getDataField();
    
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

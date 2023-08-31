/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LOCATIONS_H_
#define LOCATIONS_H_

#include "Types.h"
#include "Location.h"
#include "DiseaseModel.h"
#include "Location.h"
#include "contact_model/ContactModel.h"

#include <vector>
#include <set>
#include <string>
#include <unordered_map>
#include <iostream>

class Locations : public CBase_Locations {
 private:
  Id numLocalLocations;
  Id firstLocalLocationIdx;
  std::vector<Location> locations;
  DiseaseModel *diseaseModel;
  ContactModel *contactModel;
  std::ofstream *interactionsFile;
  int day;

  // For random generation.
  static std::uniform_real_distribution<> unitDistrib;
  std::default_random_engine generator;

  // Each Event in one of these containers is the arrival event for a
  // a person at a location
  std::vector<Event> infectiousArrivals;
  std::vector<Event> susceptibleArrivals;

  // Maps each susceptible person's id to a list of interactions with people
  // who could have infected them
  std::unordered_map<Id, std::vector<Interaction> > interactions;

  // Runs through all of the current events and return the indices of
  // any people who have been infected
  Counter processEvents(Location *loc);

  // Helper functions to handle when a person leaves a location
  // onDeparture branches to one of the two other functions
  inline void onDeparture(Location *loc, const Event& departure);
  void onSusceptibleDeparture(Location *loc, const Event& departure);
  void onInfectiousDeparture(Location *loc, const Event& departure);

  // Helper function which packages all the neccessary information about
  // an interaction between a susceptible person and an infectious person
  // and add it to the approriate list for the susceptible person
  inline void registerInteraction(Location *loc, const Event &susceptibleEvent,
    const Event &infectiousEvent, Time startTime, Time endTime);

  // Simple helper function which send the list of interactions with the
  // specified person to the appropriate People chare
  inline void sendInteractions(Location *loc, Id personIdx);

  #if ENABLE_DEBUG >= DEBUG_VERBOSE
  Counter saveInteractions(const Location &loc, const Event &departure,
    std::ofstream *out) const;
  #endif

 public:
  explicit Locations(int seed, std::string scenarioPath);
  explicit Locations(CkMigrateMessage *msg);
  void pup(PUP::er &p);  // NOLINT(runtime/references)
  void ReceiveVisitMessages(VisitMessage visitMsg);
  void ComputeInteractions();  // calls ReceiveInfections
  void ReceiveIntervention(PartitionId interventionIdx);
  // Load location data from CSV.
  void loadLocationData(std::string scenarioPath);
  #ifdef ENABLE_LB
  void ResumeFromSync();
  #endif  // ENABLE_LB
};

#endif  // LOCATIONS_H_

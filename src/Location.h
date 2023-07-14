/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LOCATION_H_
#define LOCATION_H_

// Foreward declaration to help with includes
class Location;

#include "Event.h"
#include "intervention_model/AttributeTable.h"
#include "readers/DataInterface.h"

#include <vector>
#include <functional>
#include <random>
#include <set>
#include <unordered_map>

// Represents a single location where people can interact
// Not to be confused with Locations, which represents a group of
// intances of this class
class Location : public DataInterface {
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
  explicit Location(CkMigrateMessage *msg);
  Location(int uniqueIdx, const AttributeTable &attributes);
  Location(const Location&) = default;
  Location(Location&&) = default;
  ~Location() = default;

  // Default assignment operators.
  Location& operator=(const Location&) = default;
  Location& operator=(Location&&) = default;

  // Lets us migrate these objects
  void pup(PUP::er &p);  // NOLINT(runtime/references)

  // Adds an event represnting a person either arriving or departing
  // from this location
  void addEvent(Event e);
};

#endif  // LOCATION_H_

/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LOCATION_H_
#define LOCATION_H_

// Foreward declaration to help with includes
class Location;

#include "Types.h"
#include "Event.h"
#include "readers/AttributeTable.h"
#include "readers/DataInterface.h"

#include <vector>
#include <functional>
#include <random>
#include <set>
#include <unordered_map>

// Represents a single location where people can interact
// Not to be confused with Locations, which represents a group of
// instances of this class
class Location : public DataInterface {
 public:
  // Represents all of the arrivals and departures of people
  // from this location on a given day
  std::vector<Event> events;
  std::unordered_map<const void *, VisitTest> visitFilters;
#ifdef ENABLE_SC
  bool anyInfectious;
#endif
  // Integer byte offsets in visits file by day.
  // For example, fseek(visitOffsetByDay[2]) would seek to the start
  // of this person's visits on day 3.
  std::vector<CacheOffset> visitOffsetByDay;

  // Holds visit messages for each day
  std::vector<std::vector<VisitMessage> > visitsByDay;

  // This distribution should always be the same - not sure how well
  // static variables work with Charm++, so this may need to be put
  // on the stack somewhere later on
  // static std::uniform_real_distribution<> unitDistrib;
  // Provide default constructor operations.
  Location() = default;
  explicit Location(CkMigrateMessage *msg);
  Location(const AttributeTable &attributes, int numInterventions,
    int uniqueId, int numDays);
  Location(const Location&) = default;
  Location(Location&&) = default;
  ~Location() = default;

  // Default assignment operators.
  Location& operator=(const Location&) = default;
  Location& operator=(Location&&) = default;

  // Lets us migrate these objects
  void pup(PUP::er &p);  // NOLINT(runtime/references)

  // Clear any state kept for tracking a specific day's visits
  void reset();

  // Adds an event representing a person either arriving or departing
  // from this location
  void addEvent(const Event &e);
  void filterVisits(const void *cause, VisitTest keepVisit) override;
  void restoreVisits(const void *cause) override;
  bool acceptsVisit(const VisitMessage &visit) const;
};

#endif  // LOCATION_H_

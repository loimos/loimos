/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "Types.h"
#include "Location.h"
#include "Event.h"
#include "Defs.h"
#include "readers/AttributeTable.h"

#ifdef USE_HYPERCOMM
  #include "Aggregator.h"
#endif  // USE_HYPERCOMM

#include <random>
#include <vector>
#include <cmath>
#include <utility>
#include <algorithm>

Location::Location(const AttributeTable &attributes,
    int numInterventions, int uniqueId_, int numDaysWithDistinctVisits) :
    DataInterface(attributes, numInterventions) {
  setUniqueId(uniqueId_);
  eventsByDay.resize(numDaysWithDistinctVisits);
  reset();
}

Location::Location(CkMigrateMessage *msg) {}

void Location::pup(PUP::er &p) {
  p | data;
  p | uniqueId;
  p | eventsByDay;
  p | generator;
  p | updated;
}

void Location::reset() {
  updated = false;
}

// Event processing.
void Location::addEvent(int day, const Event &e) {
  eventsByDay[day].push_back(e);
  updated = true;
}

void Location::filterVisits(const void *cause, VisitTest keepVisit) {
  visitFilters[cause] = keepVisit;
}

void Location::restoreVisits(const void *cause) {
  visitFilters.erase(cause);
}

bool Location::acceptsVisit(const VisitMessage &visit) {
  for (const std::pair<const void *, VisitTest> &pair : visitFilters) {
    if (!pair.second(visit)) {
      return false;
    }
  }
  return true;
}

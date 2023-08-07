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
    int numInterventions, int uniqueId_) :
    DataInterface(attributes, numInterventions) {
  setUniqueId(uniqueId_);
}

Location::Location(CkMigrateMessage *msg) {}

void Location::pup(PUP::er &p) {
  p | data;
  p | uniqueId;
  p | events;
}

// Event processing.
void Location::addEvent(const Event &e) {
  events.push_back(e);
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

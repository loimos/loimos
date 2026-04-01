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
    int numInterventions, int uniqueId_, int numDays) :
    DataInterface(attributes, numInterventions) {
  setUniqueId(uniqueId_);
  reset();
  // Create an entry for each day we have data for
  visitsByDay.resize(numDays);
  visitOffsetByDay.reserve(numDays);
}

Location::Location(CkMigrateMessage *msg) {}

void Location::pup(PUP::er &p) {
  p | data;
  p | uniqueId;
  p | events;
  p | generator;
  p | visitOffsetByDay;
  p | visitsByDay;
#ifdef ENABLE_SC
  p | anyInfectious;
#endif
}

void Location::reset() {
#ifdef ENABLE_SC
  anyInfectious = false;
#endif
  events.clear();
}

// Event processing.
void Location::addEvent(const Event &e) {
  events.push_back(e);
}

void Location::filterVisits(InterventionId interventionId, VisitTest keepVisit) {
  for (std::vector<VisitMessage> &visits : visitsByDay) {
    for (int i = 0; i < visits.size(); ++i) {
      if (!keepVisit(visits[i])) {
        visits[i].deactivatedBy = interventionId;
      }
    }
  }
}

void Location::restoreVisits(InterventionId interventionId, VisitTest restoreVisit) {
  for (std::vector<VisitMessage> &visits : visitsByDay) {
    for (int i = 0; i < visits.size(); ++i) {
      if (interventionId == visits[i].deactivatedBy && restoreVisit(visits[i])) {
        visits[i].deactivatedBy = DEACTIVATED_BY_NONE;
      }
    }
  }
}

bool Location::acceptsVisit(const VisitMessage &visit) {
  for (const std::pair<const void *, VisitTest> &pair : visitFilters) {
    if (!pair.second(visit)) {
      return false;
    }
  }
  return true;
}

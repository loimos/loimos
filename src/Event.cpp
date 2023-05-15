/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "Event.h"

// This is just so that we can order Events in the Location queues
bool Event::operator<(const Event& rhs) const {
  // First compare times...
  if (scheduledTime != rhs.scheduledTime) {
    return scheduledTime < rhs.scheduledTime;
  }

  // .. then break ties with the type...
  if (type != rhs.type) {
    return type < rhs.type;
  }

  // ...then break ties with the visitor's index...
  if (personIdx != rhs.personIdx) {
    return personIdx < rhs.personIdx;
  }

  // ...then finally break ties with the visitor's state
  return personState < rhs.personState;
}

// Compares two events based on the correspodning other event (if one is an
// arrival, the matching departure, and vice versa)
bool Event::greaterPartner(const Event &e0, const Event &e1) {
  // First compare times...
  if (e0.partnerTime != e1.partnerTime) {
    return e0.partnerTime > e1.partnerTime;
  }

  // ...then break ties with the types...
  if (e0.type != e1.type) {
    // equivalent of comparing the opposite types
    return e0.type < e1.type;
  }

  // ...then break ties with the visitor's index...
  if (e0.personIdx != e1.personIdx) {
    return e0.personIdx > e1.personIdx;
  }

  // ...and finally breka ties with the vistior's state
  return e0.personState > e1.personState;
}

bool Event::overlap(const Event &e0, const Event &e1) {
  int start0, end0, start1, end1;
  if (ARRIVAL == e0.type) {
    start0 = e0.scheduledTime;
    end0 = e0.partnerTime;
  } else {
    end0 = e0.scheduledTime;
    start0 = e0.partnerTime;
  }
  if (ARRIVAL == e1.type) {
    start1 = e1.scheduledTime;
    end1 = e1.partnerTime;
  } else {
    end1 = e1.scheduledTime;
    start1 = e1.partnerTime;
  }

  return (start0 <= start1 && end0 > start1)
    || (start0 > start1 && end1 > start0);
}

void Event::pair(Event *e0, Event *e1) {
  e0->partnerTime = e1->scheduledTime;
  e1->partnerTime = e0->scheduledTime;
}

/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "Event.h"

// This is just so that we can order Events in the Location queues
bool Event::operator>(const Event& rhs) const {
  if (scheduledTime != rhs.scheduledTime) {
    return scheduledTime > rhs.scheduledTime;
  }

  if (type != rhs.type) {
    return type > rhs.type;
  }

  if (personIdx != rhs.personIdx) {
    return personIdx > rhs.personIdx;
  }

  return personState > rhs.personState;
}

bool Event::greaterPartner(Event e0, Event e1) {
  if (e0.partnerTime != e1.partnerTime) {
    return e0.partnerTime > e1.partnerTime;
  }

  if (e0.type != e1.type) {
    // equivalent of comparing the opposite types
    return e0.type < e1.type;
  }

  if (e0.personIdx != e1.personIdx) {
    return e0.personIdx > e1.personIdx;
  }

  return e0.personState > e1.personState;
}

void Event::pair(Event *e0, Event *e1) {
  e0->partnerTime = e1->scheduledTime;
  e1->partnerTime = e0->scheduledTime;
}

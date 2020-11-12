/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

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
  return e0.partner > e1.partner;
}

void Event::pair(Event *e0, Event *e1) {
  e0->partner = e1;
  e1->partner = e0;
}

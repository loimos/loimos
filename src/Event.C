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

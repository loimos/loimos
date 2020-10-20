/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "Event.h"

// This is just so that we can order Events in the Location queues
bool Event::operator>(const Event& rhs) const {
  return time > rhs.time;
}

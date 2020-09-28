/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "Location.h"
#include "Event.h"
#include "Defs.h"

void Location::addEvent(Event e) {
  events.push(e);
}

void Location::processEvents() {
  std::vector<int> people;
  Event curEvent;
  while (!events.empty()) {
    curEvent = events.top();
    events.pop();

    // TODO: implement a disease model to make this check properly
    if (HEALTHY == curEvent.personState) {
      people = susceptiblePeople;
    } else if (INFECTED == curEvent.personState) {
      people = infectiousPeople;
    } else {
      continue;
    }

    if (ARRIVAL == curEvent.type) {
      people.push_back(curEvent.personIdx);

    } else if (DEPARTURE == curEvent.type) {
      people.erase(
        std::remove(
          people.begin(),
          people.end(),
          curEvent.personIdx
        ), people.end()
      );
    }
  }
}

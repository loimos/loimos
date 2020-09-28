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
  
  //TODO: implement location infectivity
  //if (diseaseModel->isInfectious(personState))
  //  locationState[localLocIdx] = INFECTIOUS;
}

void Location::processEvents() {
  Event curEvent;
  while (!events.empty()) {
    curEvent = events.top();
    events.pop();

    // TODO: handle event
  }
}

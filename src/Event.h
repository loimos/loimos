/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef EVENT_H_
#define EVENT_H_

#include "charm++.h"
#include "Defs.h"

// This is just a bundle of information that we don't need to
// guarentee any constraints on, hence why this is a stuct rather than
// a class
struct Event {
  // indicates whether they're arriving or leaving
  EventType type;
  // the index of the person arriving or leaving
  int personIdx;
  // the person's curent state in the disease model
  int personState;
  // Susceptibility or infectivity, depending on disease state
  double transmissionModifier;
  // the time when this event is scheduled to occur, in seconds from the
  // start of the day
  int scheduledTime;
  // if this is an arrival, the time of the corresponding departure,
  // and vice versa
  int partnerTime;

  // Lets us order events in the location queues
  bool operator<(const Event& rhs) const;

  // Compares events based on their partners, returning whether or not e0's
  // partner is greater than e1's. Assumes both partnes are non-null
  static bool greaterPartner(const Event &e0, const Event &e1);

  static bool overlap(const Event &e0, const Event &e1);

  // Makes two events each others' partners
  static void pair(Event *e0, Event *e1);
};
PUPbytes(Event);

#endif  // EVENT_H_

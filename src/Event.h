/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __EVENT_H__
#define __EVENT_H__

// This is just a bundle of information that we don't need to
// guarentee any constraints on, hence why this is a stuct rather than
// a class
struct Event {
  // the index of the person arriving or leaving
  int personIdx;
  // the person's curent state in the disease model
  int personState;
  // the time when this event occurs, in seconds from the start of the day
  int time;
  // indicates whether they're arriving or leaving
  char type;

  // Lets us order events in the location queues
  bool operator>(const Event& rhs) const;
};

#endif // __EVENT_H__

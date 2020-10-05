/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __PERSON_H__
#define __PERSON_H__

#include "Defs.h"

// This is just a bundle of information that we don't need to
// guarentee any constraints on, hence why this is a stuct rather than
// a class
struct Person {
  // the person's curent state in the disease model
  int state;
  // how long until the person transitions to their next state
  int secondsLeftInState;
};

#endif // __PERSON_H__

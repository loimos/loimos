/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "Interaction.h"

void Interaction::pup(PUP::er &p) {
  p|propensity;
  p|infectiousIdx;
  p|infectiousState;
  //p|targetState;
  p|startTime;
  p|endTime;
}

/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "Interaction.h"

Interaction::Interaction(
  double propensity,
  int infectiousIdx,
  int infectiousState,
  int startTime,
  int endTime
) :
  propensity(propensity),
  infectiousIdx(infectiousIdx),
  infectiousState(infectiousState),
  startTime(startTime),
  endTime(endTime)
{}

void Interaction::pup(PUP::er &p) {
  p|propensity;
  p|infectiousIdx;
  p|infectiousState;
  //p|targetState;
  p|startTime;
  p|endTime;
}

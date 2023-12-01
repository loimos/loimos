/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef INTERACTION_H_
#define INTERACTION_H_

#include "charm++.h"
#include "Types.h"

// Simple struct to hold data on an interfaction between with a susceptible
// person which could lead to an infection
struct Interaction {
  // Describes the chance of this interaction resulting in an infection
  double propensity;
  // Data on the person who could potentially infect the susceptible person in
  // question
  Id infectiousIdx;
  DiseaseState infectiousState;
  // We need to know when the interaction occured so that, if this interaction
  // does in fact result in an infection, we can determine precisely when it
  // occurred
  Time startTime;
  Time endTime;

  Interaction() {}
  Interaction(double propensity_, Id infectiousIdx_,
      DiseaseState infectiousState_, Time startTime_, Time endTime_) :
    propensity(propensity_), infectiousIdx(infectiousIdx_),
    infectiousState(infectiousState_), startTime(startTime_),
    endTime(endTime_) {}
  explicit Interaction(CkMigrateMessage *msg) {}

  void pup(PUP::er& p) {  // NOLINT(runtime/references)
    p | propensity;
    p | infectiousIdx;
    p | infectiousState;
    p | startTime;
    p | endTime;
  }
};
PUPbytes(Interaction);

#endif  // INTERACTION_H_

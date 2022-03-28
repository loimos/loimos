/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include "Interaction.h"
#include "pup_stl.h"

struct VisitMessage {
  int locationIdx;
  int personIdx;
  int personState;
  int visitStart;
  int visitEnd;

  VisitMessage() {}
  VisitMessage(CkMigrateMessage *msg) {}
  VisitMessage(int locationIdx_, int personIdx_, int personState_, int visitStart_,
      int visitEnd_) : locationIdx(locationIdx_), personIdx(personIdx_),
      personState(personState_), visitStart(visitStart_), visitEnd(visitEnd_) {}
};
PUPbytes(VisitMessage);

struct InteractionMessage {
  int personIdx;
  std::vector<Interaction> interactions;

  InteractionMessage() {}
  InteractionMessage(CkMigrateMessage *msg) {}
  InteractionMessage(int personIdx_, const std::vector<Interaction>& interactions_)
    : personIdx(personIdx_), interactions(interactions_) {}

  void pup(PUP::er& p) {
    p | personIdx;
    p | interactions;
  }
};

#endif // __MESSAGE_H__

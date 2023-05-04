/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "Interaction.h"
#include "pup_stl.h"

#include <vector>

struct VisitMessage {
  int locationIdx;
  int personIdx;
  int personState;
  int visitStart;
  int visitEnd;

  VisitMessage() {}
  explicit VisitMessage(CkMigrateMessage *msg) {}
  VisitMessage(int locationIdx_, int personIdx_, int personState_, int visitStart_,
      int visitEnd_) : locationIdx(locationIdx_), personIdx(personIdx_),
      personState(personState_), visitStart(visitStart_), visitEnd(visitEnd_) {}
};
PUPbytes(VisitMessage);

struct InteractionMessage {
  int locationIdx;
  int personIdx;
  std::vector<Interaction> interactions;

  InteractionMessage() {}
  explicit InteractionMessage(CkMigrateMessage *msg) {}
  InteractionMessage(int locationIdx_, int personIdx_,
      const std::vector<Interaction>& interactions_)
    : locationIdx(locationIdx_), personIdx(personIdx_),
    interactions(interactions_) {}

  void pup(PUP::er& p) {  // NOLINT(runtime/references)
    p | locationIdx;
    p | personIdx;
    p | interactions;
  }
};

#endif  // MESSAGE_H_

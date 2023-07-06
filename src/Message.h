/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "Interaction.h"
#include "pup_stl.h"
#include <functional>
typedef int (*func)(int);

#include <vector>

struct VisitMessage {
  int locationIdx;
  int personIdx;
  int personState;
  int visitStart;
  int visitEnd;

  VisitMessage() {}
  explicit VisitMessage(CkMigrateMessage *msg) {}
  VisitMessage(int locationIdx_, int personIdx_, int personState_,
      int visitStart_, int visitEnd_) : locationIdx(locationIdx_),
      personIdx(personIdx_), personState(personState_),
      visitStart(visitStart_), visitEnd(visitEnd_) {}
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

struct InterventionMessage {
  int personIdx;
  int attrIndex;
  double newValue;

  InterventionMessage() {}
  explicit InterventionMessage(CkMigrateMessage *msg) {}
  InterventionMessage(int personIdx_, int attrIndex_, double newValue_)
    : personIdx(personIdx_), attrIndex(attrIndex_), newValue(newValue_) {}

  void pup(PUP::er& p) {
    p | personIdx;
    p | attrIndex;
    p | newValue;
  }
};

#endif  // MESSAGE_H_

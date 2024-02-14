/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "Types.h"
#include "Interaction.h"
#include "pup_stl.h"

#include <vector>
#include <functional>

struct VisitMessage {
  Id locationIdx;
  Id personIdx;
  Time visitStart;
  Time visitEnd;
  const void *deactivatedBy;

  VisitMessage() {}
  explicit VisitMessage(CkMigrateMessage *msg) {}
  VisitMessage(Id locationIdx_, Id personIdx_, Time visitStart_,
      Time visitEnd_) :
    locationIdx(locationIdx_), personIdx(personIdx_),
    visitStart(visitStart_), visitEnd(visitEnd_), deactivatedBy(NULL) {}

  bool isActive() {
    return NULL != deactivatedBy;
  }

  void checkTimes(PartitionId partition) {
    if (visitStart > visitEnd) {
      CkAbort("Error on chare %d: visit by " ID_PRINT_TYPE " to loc " ID_PRINT_TYPE "\n"
        "has departure (%d) before arrival (%d)\n",
        partition, personIdx, locationIdx, visitEnd,
        visitStart);
    }
  }
};
PUPbytes(VisitMessage);

using VisitTest = std::function<bool(const VisitMessage &)>;

struct StateMessage {
  Id personIdx;
  DiseaseState state;
  // Susceptibility or infectivity, depending on disease state
  double transmissionModifier;

  StateMessage() {}
  explicit StateMessage(CkMigrateMessage *msg) {}
  StateMessage(Id personIdx_, DiseaseState state_,
      double transmissionModifier_) :
    personIdx(personIdx_), state(state_),
    transmissionModifier(transmissionModifier_) {}

  // Lets us order events in a set
  bool operator<(const StateMessage& rhs) const {
    if (personIdx != rhs.personIdx) {
      return personIdx < rhs.personIdx;
    }

    if (state != rhs.state) {
      return state < rhs.state;
    }

    return transmissionModifier < rhs.transmissionModifier;
  }
};
PUPbytes(StateMessage);

struct InteractionMessage {
  Id locationIdx;
  Id personIdx;
  std::vector<Interaction> interactions;

  InteractionMessage() {}
  explicit InteractionMessage(CkMigrateMessage *msg) {}
  InteractionMessage(Id locationIdx_, Id personIdx_,
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

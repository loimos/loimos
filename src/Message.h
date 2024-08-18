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
#include <unordered_set>
#include <functional>

struct VisitMessage {
  Id locationIdx;
  Id personIdx;
  DiseaseState personState;
  Time visitStart;
  Time visitEnd;
  // Susceptibility or infectivity, depending on disease state
  double transmissionModifier;
  const void *deactivatedBy;

  VisitMessage() {}
  explicit VisitMessage(CkMigrateMessage *msg) {}
  VisitMessage(Id locationIdx_, Id personIdx_, DiseaseState personState_,
      Time visitStart_, Time visitEnd_, double transmissionModifier_) :
    locationIdx(locationIdx_), personIdx(personIdx_),
    personState(personState_), visitStart(visitStart_),
    visitEnd(visitEnd_), transmissionModifier(transmissionModifier_),
    deactivatedBy(NULL) {}

  bool isActive() {
    return NULL != deactivatedBy;
  }
};
PUPbytes(VisitMessage);

using VisitTest = std::function<bool(const VisitMessage &)>;

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

struct ExpectedVisitorsMessage {
  PartitionId destPartition;
  std::unordered_set<Id> visitors;
  
  ExpectedVisitorsMessage() {}
  explicit ExpectedVisitorsMessage(CkMigrateMessage *msg) {}
  ExpectedVisitorsMessage(PartitionId destPartition_)
    : destPartition(destPartition_) {}
  
  void pup(PUP::er& p) {  // NOLINT(runtime/references)
    p | destPartition;
    p | visitors;
  }
};

#endif  // MESSAGE_H_

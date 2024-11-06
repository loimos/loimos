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
#include <limits>

const InterventionId DEACTIVATED_BY_NONE = -1;
struct VisitMessage {
  Id locationIdx;
  Id personIdx;
  DiseaseState personState;
  Time visitStart;
  Time visitEnd;
  // Susceptibility or infectivity, depending on disease state
  double transmissionModifier;
  int deactivatedBy;

  VisitMessage() {}
  explicit VisitMessage(CkMigrateMessage *msg) : deactivatedBy(DEACTIVATED_BY_NONE) {}
  VisitMessage(Id locationIdx_, Id personIdx_, DiseaseState personState_,
      Time visitStart_, Time visitEnd_, double transmissionModifier_) :
    locationIdx(locationIdx_), personIdx(personIdx_),
    personState(personState_), visitStart(visitStart_),
    visitEnd(visitEnd_), transmissionModifier(transmissionModifier_),
    deactivatedBy(DEACTIVATED_BY_NONE) {}

  bool isActive() const {
    return DEACTIVATED_BY_NONE == deactivatedBy;
  }
};
PUPbytes(VisitMessage);

struct VisitScheduleMessage {
  PartitionId sourcePartition;
  std::vector<std::vector<VisitMessage> > visitsByDay;

  VisitScheduleMessage() {}
  explicit VisitScheduleMessage(CkMigrateMessage *msg) {}
  explicit VisitScheduleMessage(PartitionId sourcePartition_)
    : sourcePartition(sourcePartition_) {}

  void pup(PUP::er& p) {  // NOLINT(runtime/references)
    p | sourcePartition;
    p | visitsByDay;
  }
};

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
  explicit ExpectedVisitorsMessage(PartitionId destPartition_)
    : destPartition(destPartition_) {}

  void pup(PUP::er& p) {  // NOLINT(runtime/references)
    p | destPartition;
    p | visitors;
  }
};

struct PersonState {
  Id uniqueId;
  DiseaseState state;
  double transmissionModifier;

  PersonState() {}
  explicit PersonState(CkMigrateMessage *msg) {}
  PersonState(Id uniqueId_, DiseaseState state_,
    double transmissionModifier_)
    : uniqueId(uniqueId_), state(state_),
    transmissionModifier(transmissionModifier_) {}
};
PUPbytes(PersonState);

struct PersonStatesMessage {
  PartitionId sourcePartition;
  std::vector<PersonState> states;

  PersonStatesMessage() {}
  explicit PersonStatesMessage(CkMigrateMessage *msg) {}
  explicit PersonStatesMessage(PartitionId sourcePartition_)
    : sourcePartition(sourcePartition_) {}

  void pup(PUP::er& p) {  // NOLINT(runtime/references)
    p | sourcePartition;
    p | states;
  }
};

struct VisitInterventionMessage {
  int interventionIdx;
  std::unordered_set<Id> affectedPeople;
  std::unordered_set<Id> previouslyAffectedPeople;

  VisitInterventionMessage() {}
  VisitInterventionMessage(PartitionId interventionIdx_,
      const std::unordered_set<Id>& affectedPeople_,
      const std::unordered_set<Id>& previouslyAffectedPeople_)
    : interventionIdx(interventionIdx_),
    affectedPeople(affectedPeople_),
    previouslyAffectedPeople(previouslyAffectedPeople_) {}

  void pup(PUP::er& p) {  // NOLINT(runtime/references)
    p | interventionIdx;
    p | affectedPeople;
    p | previouslyAffectedPeople;
  }
};

#endif  // MESSAGE_H_

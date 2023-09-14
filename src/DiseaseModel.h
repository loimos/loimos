/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef DISEASEMODEL_H_
#define DISEASEMODEL_H_

#include "Event.h"
#include "Person.h"
#include "Location.h"
#include "Types.h"
#include "protobuf/disease.pb.h"
#include "protobuf/distribution.pb.h"
#include "protobuf/data.pb.h"
#include "protobuf/interventions.pb.h"
#include "readers/DataReader.h"
#include "readers/DataInterface.h"
#include "readers/AttributeTable.h"
#include "intervention_model/Intervention.h"

#include <unordered_map>
#include <random>
#include <tuple>
#include <string>
#include <vector>
#include <memory>

using NameIndexLookupType = std::unordered_map<std::string, int>;

class DiseaseModel : public CBase_DiseaseModel {
 private:
  loimos::proto::DiseaseModel *model;
  Time getTimeInNextState(const
    loimos::proto::DiseaseModel_DiseaseState_TimedTransitionSet_StateTransition
      *transitionSet, std::default_random_engine *generator) const;
  Time timeDefToSeconds(TimeDef time) const;
  
  // Offset-based index lookup
  std::vector<Id> locationPartitionOffsets;
  std::vector<Id> personPartitionOffsets;
  void setPartitionOffsets(PartitionId numPartitions, Id numObjects,
    loimos::proto::CSVDefinition *metadata, std::vector<Id> *partitionOffsets);

  // Intervention related.
  std::vector<bool> triggerFlags;
  std::vector<std::shared_ptr<Intervention<Person>>> personInterventions;
  std::vector<std::shared_ptr<Intervention<Location>>> locationInterventions;

  void intitialisePersonInterventions(
    const InterventionList &interventionSpecs,
    const AttributeTable &attributes);
  void intitialiseLocationInterventions(
    const InterventionList &interventionSpecs,
    const AttributeTable &attributes);

 public:
  // move back to private later
  DiseaseModel(std::string pathToModel, std::string scenarioPath,
      std::string pathToIntervention);
  DiseaseState getIndexOfState(std::string stateLabel) const;
  // TODO(iancostello): Change interventionStategies to index based.
  std::tuple<DiseaseState, Time> transitionFromState(DiseaseState fromState,
    std::default_random_engine *generator) const;
  std::string lookupStateName(DiseaseState state) const;
  int getNumberOfStates() const;
  DiseaseState getHealthyState(const std::vector<Data> &dataField) const;
  bool isInfectious(DiseaseState personState) const;
  bool isSusceptible(DiseaseState personState) const;
  const char * getStateLabel(DiseaseState personState) const;
  double getLogProbNotInfected(Event susceptibleEvent, Event infectiousEvent) const;
  double getPropensity(
      DiseaseState susceptibleState,
      DiseaseState infectiousState,
      Time startTime,
      Time endTime,
      double susceptibility,
      double infectivity) const;

  // These objects are not related to the disease model but are
  // per PE definitions so it makes sense to share them.
  loimos::proto::CSVDefinition *personDef;
  loimos::proto::CSVDefinition *locationDef;
  loimos::proto::CSVDefinition *activityDef;
  loimos::proto::InterventionModel *interventionDef;

  // Intervention methods
  int susceptibilityIndex;
  int infectivityIndex;
  AttributeTable personAttributes;
  AttributeTable locationAttributes;

  // Offset-based index interface - each calls the corresponding function
  // with the corresponding offset vector
  Id getLocalLocationIndex(Id globalIndex, PartitionId PartitionId) const;
  Id getGlobalLocationIndex(Id localIndex, PartitionId PartitionId) const;
  PartitionId getLocationPartitionIndex(Id globalIndex) const;
  Id getLocationPartitionSize(PartitionId partitionIndex) const;
  Id getLocalPersonIndex(Id globalIndex, PartitionId PartitionId) const;
  Id getGlobalPersonIndex(Id localIndex, PartitionId PartitionId) const;
  PartitionId getPersonPartitionIndex(Id globalIndex) const;
  Id getPersonPartitionSize(PartitionId partitionIndex) const;

  const Intervention<Person> &getPersonIntervention(int index) const;
  const Intervention<Location> &getLocationIntervention(int index) const;
  int getNumPersonInterventions() const;
  int getNumLocationInterventions() const;
  void applyInterventions(int day, Id newDailyInfections);
  void toggleInterventions(int day, Id newDailyInfections);
};

#endif  // DISEASEMODEL_H_

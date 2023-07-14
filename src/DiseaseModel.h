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
#include "protobuf/disease.pb.h"
#include "protobuf/distribution.pb.h"
#include "protobuf/data.pb.h"
#include "protobuf/interventions.pb.h"
#include "readers/DataReader.h"
#include "readers/DataInterface.h"
#include "intervention_model/AttributeTable.h"
#include "intervention_model/Intervention.h"
#include "Event.h"

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
  // Map from state name to index of state in model.
  // TODO(iancostello): Change these maps to be non-pointers.
  std::unordered_map<std::string, int> *stateLookup;
  // For each state index, map from stategy name string to index of strategy labels.
  std::vector<std::unordered_map<std::string, int> *> *strategyLookup;
  Time getTimeInNextState(const
    loimos::proto::DiseaseModel_DiseaseState_TimedTransitionSet_StateTransition
      *transitionSet,
    std::default_random_engine *generator) const;
  Time timeDefToSeconds(TimeDef time) const;

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
  int getIndexOfState(std::string stateLabel) const;
  // TODO(iancostello): Change interventionStategies to index based.
  std::tuple<int, int> transitionFromState(int fromState,
    std::default_random_engine *generator) const;
  std::string lookupStateName(int state) const;
  int getNumberOfStates() const;
  int getHealthyState(const std::vector<Data> &dataField) const;
  bool isInfectious(int personState) const;
  bool isSusceptible(int personState) const;
  const char * getStateLabel(int personState) const;
  double getLogProbNotInfected(Event susceptibleEvent, Event infectiousEvent) const;
  double getPropensity(
      int susceptibleState,
      int infectiousState,
      int startTime,
      int endTime) const;

  // These objects are not related to the disease model but are
  // per PE definitions so it makes sense to share them.
  loimos::proto::CSVDefinition *personDef;
  loimos::proto::CSVDefinition *locationDef;
  loimos::proto::CSVDefinition *activityDef;
  loimos::proto::InterventionModel *interventionDef;

  // Intervention methods
  AttributeTable personAttributes;
  AttributeTable locationAttributes;

  const Intervention<Person> &getPersonIntervention(int index) const;
  const Intervention<Location> &getLocationIntervention(int index) const;
  void applyInterventions(int day, int newDailyInfections);
  void toggleInterventions(int day, int newDailyInfections);
};

#endif  // DISEASEMODEL_H_

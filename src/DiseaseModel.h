/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef DISEASEMODEL_H_
#define DISEASEMODEL_H_

#include "Event.h"

#include "disease_model/disease.pb.h"
#include "disease_model/distribution.pb.h"
#include "readers/DataReader.h"
#include "readers/interventions.pb.h"
#include "AttributeTable.h"

#include "Event.h"
#include "readers/data.pb.h"

#include <unordered_map>
#include <random>
#include <tuple>
#include <string>
#include <vector>

using NameIndexLookupType = std::unordered_map<std::string, int>;
using InterventionTestType = std::function<
  bool(const loimos::proto::InterventionModel::Intervention)>;

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
  std::vector<bool> isTriggered;

 public:
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
  void toggleIntervention(int day, int newDailyInfections);
  int getInterventionIndex(InterventionTestType test) const;
  double getCompliance(int interventionIndex) const;
  bool shouldPersonIsolate(int healthState) const;
  bool isLocationOpen(std::vector<Data> *locAttr) const;
  bool complyingWithLockdown(std::default_random_engine *generator) const;

  // Tables containing lookup information for attributes
  AttributeTable personTable{1};
  AttributeTable locationTable{0};
  // Populate in diseaseModel constructor, don't define here
};

#endif  // DISEASEMODEL_H_

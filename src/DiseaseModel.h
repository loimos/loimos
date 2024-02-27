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
  Time getTimeInNextState(const
    loimos::proto::DiseaseModel_DiseaseState_TimedTransitionSet_StateTransition
      *transitionSet, std::default_random_engine *generator, Id *numPeople,
      Id *numLocations) const;
  Time timeDefToSeconds(TimeDef time) const;
  Time timeDefToDays(TimeDef time) const;

 public:
  loimos::proto::DiseaseModel *model;

  DiseaseModel(std::string diseasePath);
  DiseaseState getIndexOfState(std::string stateLabel) const;
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
};

#endif  // DISEASEMODEL_H_

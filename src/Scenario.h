/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef SCENARIO_H__
#define SCENARIO_H__

#include "Event.h"
#include "Person.h"
#include "Location.h"
#include "Types.h"
#include "protobuf/disease.pb.h"
#include "protobuf/distribution.pb.h"
#include "protobuf/data.pb.h"
#include "protobuf/interventions.pb.h"
#include "Partitioner.h"
#include "DiseaseModel.h"
#include "readers/DataReader.h"
#include "readers/DataInterface.h"
#include "readers/AttributeTable.h"
#include "intervention_model/Intervention.h"
#include "contact_model/ContactModel.h"

#include <unordered_map>
#include <random>
#include <tuple>
#include <string>
#include <vector>
#include <memory>

struct InterventionModel {
  std::vector<bool> triggerFlags;
  std::vector<std::shared_ptr<Intervention<Person>>> personInterventions;
  std::vector<std::shared_ptr<Intervention<Location>>> locationInterventions;
  loimos::proto::InterventionModel *interventionDef;

  InterventionModel();
  InterventionModel(std::string interventionPath,
    AttributeTable *personAttributes, AttributeTable *locationAttributes,
    const DiseaseModel &diseaseModel);
  void initPersonInterventions(
    const InterventionList &interventionSpecs,
    const AttributeTable &attributes, const DiseaseModel &diseaseModel);
  void initLocationInterventions(
    const InterventionList &interventionSpecs,
    const AttributeTable &attributes, const DiseaseModel &diseaseModel);

  const Intervention<Person> &getPersonIntervention(int index) const;
  const Intervention<Location> &getLocationIntervention(int index) const;
  int getNumPersonInterventions() const;
  int getNumLocationInterventions() const;
  void applyInterventions(int day, Id newDailyInfections, Id numPeople);
  void toggleInterventions(int day, Id newDailyInfections, Id numPeople);
};

class Scenario : public CBase_Scenario {
 public:
  int seed;
  const Time numDays;
  const Time numDaysWithDistinctVisits;
  const Time numDaysToSeedOutbreak;
  const Id numInitialInfectionsPerDay;
  Id numPeople;
  Id numLocations;

  const std::string scenarioPath;
  const std::string outputPath;
  std::string scenarioId;

  loimos::proto::CSVDefinition *personDef;
  loimos::proto::CSVDefinition *locationDef;
  loimos::proto::CSVDefinition *visitDef;

  AttributeTable personAttributes;
  AttributeTable locationAttributes;

  OnTheFlyArguments *onTheFly;
  Partitioner *partitioner;
  DiseaseModel *diseaseModel;
  ContactModel *contactModel;
  InterventionModel *interventionModel;

  explicit Scenario(Arguments args);
  void applyInterventions(int day, Id newDailyInfections);
  bool isOnTheFly();
  bool hasInterventions();
};

#endif  // SCENARIO_H__

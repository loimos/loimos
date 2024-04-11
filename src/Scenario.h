/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef SCENARIO_H__
#define SCENARIO_H__

#include "Types.h"
#include "Event.h"
#include "Person.h"
#include "Location.h"
#include "protobuf/data.pb.h"
#include "Partitioner.h"
#include "DiseaseModel.h"
#include "contact_model/ContactModel.h"
#include "intervention_model/InterventionModel.h"

#include <string>

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

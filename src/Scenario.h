/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef SCENARIO_H__
#define SCENARIO_H__

#include "readers/Parse.h"
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

#ifdef ENABLE_LB
  int lb_start_day;
  int lb_interval;
#endif

#ifdef ENABLE_TRACING
  int tracing_start_day;
  int tracing_end_day;
#endif

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

  Scenario()
          : seed(0),
            numDays(0),
            numDaysWithDistinctVisits(1),
            numDaysToSeedOutbreak(0),
            numInitialInfectionsPerDay(0),
            scenarioPath(""),
            outputPath(""),
            personDef(nullptr),
            locationDef(nullptr),
            visitDef(nullptr),
            onTheFly(nullptr),
            partitioner(nullptr),
            diseaseModel(nullptr),
            contactModel(nullptr),
            interventionModel(nullptr) {}
  explicit Scenario(Arguments args);
  void ApplyInterventions(int day, Id newDailyInfections);
  bool isOnTheFly();
  bool hasInterventions();
#ifdef ENABLE_LB
  bool shouldLB(int day);
#endif
};

#endif  // SCENARIO_H__

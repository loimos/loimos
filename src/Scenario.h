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

struct Partitioner {
  Id numPeople;
  Id numLocations;
  std::vector<Id> locationPartitionOffsets;
  std::vector<Id> personPartitionOffsets;
  
  Partitioner(std::string scenarioPath,
    PartitionId numPersonPartitions,
    PartitionId numLocationPartitions,
    loimos::proto::CSVDefinition *personMetadata,
    loimos::proto::CSVDefinition *locationMetadata);
  Partitioner(PartitionId numPersonPartitions,
    PartitionId numLocationPartitions, Id numPeople,
    Id numLocations);

  static void setPartitionOffsets(PartitionId numPartitions,
    Id firstIndex, Id numObjects, loimos::proto::CSVDefinition *metadata,
    std::vector<Id> *partitionOffsets);
  static void setPartitionOffsets(PartitionId numPartitions,
    Id firstIndex, Id numObjects, std::vector<Id> *partitionOffsets);

  // Offset-based index interface - each calls the corresponding function
  // with the corresponding offset vector
  Id getLocalLocationIndex(Id globalIndex, PartitionId PartitionId) const;
  Id getGlobalLocationIndex(Id localIndex, PartitionId PartitionId) const;
  CacheOffset getLocationCacheIndex(Id globalIndex) const;
  PartitionId getLocationPartitionIndex(Id globalIndex) const;
  Id getLocationPartitionSize(PartitionId partitionIndex) const;
  PartitionId getNumLocationPartitions();

  Id getLocalPersonIndex(Id globalIndex, PartitionId PartitionId) const;
  Id getGlobalPersonIndex(Id localIndex, PartitionId PartitionId) const;
  CacheOffset getPersonCacheIndex(Id globalIndex) const;
  PartitionId getPersonPartitionIndex(Id globalIndex) const;
  Id getPersonPartitionSize(PartitionId partitionIndex) const;
  PartitionId getNumPersonPartitions();
};

struct InterventionModel {
  std::vector<bool> triggerFlags;
  std::vector<std::shared_ptr<Intervention<Person>>> personInterventions;
  std::vector<std::shared_ptr<Intervention<Location>>> locationInterventions;
  loimos::proto::InterventionModel *interventionDef;

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
  loimos::proto::DiseaseModel *diseaseDef;

  AttributeTable personAttributes;
  AttributeTable locationAttributes;

  OnTheFlyArguments *onTheFly;
  Partitioner *partitioner;
  DiseaseModel *diseaseModel;
  ContactModel *contactModel;
  InterventionModel *interventionModel;

  Scenario(Arguments args);
  void applyInterventions(int day, Id newDailyInfections);
  bool isOnTheFly();
  bool hasInterventions();
};

#endif  // SCENARIO_H__
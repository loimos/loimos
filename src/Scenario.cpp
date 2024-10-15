/* Copyright 2020-2024 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "Scenario.h"
#include "Types.h"
#include "Extern.h"
#include "Defs.h"
#include "Partitioner.h"
#include "DiseaseModel.h"
#include "contact_model/ContactModel.h"
#include "intervention_model/InterventionModel.h"
#include "readers/Preprocess.h"

#include "charm++.h"

Scenario::Scenario(Arguments args) : seed(args.seed), numDays(args.numDays),
    numDaysWithDistinctVisits(args.numDaysWithDistinctVisits),
    numDaysToSeedOutbreak(args.numDaysToSeedOutbreak),
    numInitialInfectionsPerDay(args.numInitialInfectionsPerDay),
    scenarioPath(args.scenarioPath), outputPath(args.outputPath),
    personDef(NULL), locationDef(NULL), visitDef(NULL),
    onTheFly(NULL), partitioner(NULL), diseaseModel(NULL),
    contactModel(NULL), interventionModel(NULL) {
  if (args.isOnTheFlyRun) {
    onTheFly = new OnTheFlyArguments(args.onTheFly);

    numPeople = onTheFly->personGrid.area();
    numLocations = onTheFly->locationGrid.area();

    partitioner = new Partitioner(args.numPersonPartitions,
        args.numLocationPartitions, numPeople, numLocations);
    scenarioId = std::string("");

  } else {
    // Handle people...
    personDef = new loimos::proto::CSVDefinition;
    checkReadResult(readProtobuf(args.scenarioPath + "people.textproto",
      personDef), args.scenarioPath + "people.textproto");
    numPeople = personDef->num_rows();

    loimos::proto::CSVDefinition *personOffsetDef = new loimos::proto::CSVDefinition;
    std::string personOffsetPath = args.scenarioPath + "person_offsets_" +
      std::to_string(args.partitionsToOffsetsRatio * args.numPersonPartitions) +
      ".textproto";
    if (FILE_READ_ERROR == readProtobuf(personOffsetPath, personOffsetDef)) {
      delete personOffsetDef;
      personOffsetDef = personDef;
#ifdef ENABLE_DEBUG
      CkPrintf("Read %d person offsets from %s\n",
          personOffsetDef->partition_offsets_size(),
          (args.scenarioPath + "people.textproto").c_str());
    } else {
      CkPrintf("Read %d person offsets from %s\n",
          personOffsetDef->partition_offsets_size(),
          personOffsetPath.c_str());
#endif
    }

    // ...locations...
    locationDef = new loimos::proto::CSVDefinition;
    checkReadResult(readProtobuf(args.scenarioPath + "locations.textproto",
      locationDef), args.scenarioPath + "locations.textproto");
    numLocations = locationDef->num_rows();

    loimos::proto::CSVDefinition *locationOffsetDef = new loimos::proto::CSVDefinition;
    std::string locationOffsetPath = args.scenarioPath + "location_offsets_" +
      std::to_string(args.partitionsToOffsetsRatio * args.numLocationPartitions) +
      ".textproto";
    if (FILE_READ_ERROR == readProtobuf(locationOffsetPath, locationOffsetDef)) {
      delete locationOffsetDef;
      locationOffsetDef = locationDef;
#ifdef ENABLE_DEBUG
      CkPrintf("Read %d person offsets from %s\n",
          locationOffsetDef->partition_offsets_size(),
        (args.scenarioPath + "location.textproto").c_str());
    } else {
      CkPrintf("Read %d location offsets from %s\n",
          locationOffsetDef->partition_offsets_size(),
          locationOffsetPath.c_str());
#endif
    }

    // ...and visits
    visitDef = new loimos::proto::CSVDefinition;
    checkReadResult(readProtobuf(args.scenarioPath + "visits.textproto",
      visitDef), args.scenarioPath + "visits.textproto");

    personAttributes.readAttributes(personDef->fields());
    locationAttributes.readAttributes(locationDef->fields());

    partitioner = new Partitioner(args.scenarioPath, args.numPersonPartitions,
      args.numLocationPartitions, personDef, locationDef, personOffsetDef,
      locationOffsetDef);

    if (0 == CkMyNode()) {
      buildCache(args.scenarioPath, numPeople, partitioner->personPartitionOffsets,
        numLocations, partitioner->locationPartitionOffsets, numDaysWithDistinctVisits);
    }
    scenarioId = getScenarioId(numPeople, args.numPersonPartitions,
      numLocations, args.numLocationPartitions);
  }

  diseaseModel = new DiseaseModel(args.diseasePath, args.transmissibility,
    personAttributes);
  if (args.hasIntervention) {
    interventionModel = new InterventionModel(args.interventionPath,
      &personAttributes, &locationAttributes, *diseaseModel);
  } else {
    interventionModel = new InterventionModel();
  }

  contactModel = createContactModel(args.contactModelType, locationAttributes);

#if ENABLE_DEBUG >= DEBUG_BASIC
  CkPrintf("Person Attributes:\n");
  for (int i = 0; i < personAttributes.size(); i++) {
    CkPrintf("(%d) %s: default: %lf, type: %d\n",
        i, personAttributes.getName(i).c_str(),
        personAttributes.getDefaultValueAsDouble(i),
        personAttributes.getDataType(i));
  }

  CkPrintf("Location Attributes:\n");
  for (int i = 0; i < locationAttributes.size(); i++) {
    CkPrintf("(%d) %s: default: %lf, type: %d\n",
        i, locationAttributes.getName(i).c_str(),
        locationAttributes.getDefaultValueAsDouble(i),
        locationAttributes.getDataType(i));
  }
#endif
}

void Scenario::ApplyInterventions(int day, Id newDailyInfections) {
  if (hasInterventions()) {
    interventionModel->applyInterventions(day, newDailyInfections, numPeople);
  }
}

bool Scenario::isOnTheFly() {
  return NULL != onTheFly;
}

bool Scenario::hasInterventions() {
  return NULL != interventionModel->interventionDef;
}

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
    readProtobuf(args.scenarioPath + "people.textproto", personDef);
    numPeople = personDef->num_rows();

    // ...locations...
    locationDef = new loimos::proto::CSVDefinition;
    readProtobuf(args.scenarioPath + "locations.textproto", locationDef);
    numLocations = locationDef->num_rows();

    // ...and visits
    visitDef = new loimos::proto::CSVDefinition;
    readProtobuf(args.scenarioPath + "visits.textproto", visitDef);

    personAttributes.readAttributes(personDef->fields());
    locationAttributes.readAttributes(locationDef->fields());

    partitioner = new Partitioner(args.scenarioPath, args.numPersonPartitions,
      args.numLocationPartitions, personDef, locationDef);

    if (0 == CkMyNode()) {
      buildCache(args.scenarioPath, numPeople, partitioner->personPartitionOffsets,
        numLocations, partitioner->locationPartitionOffsets, numDaysWithDistinctVisits);
    }
    scenarioId = getScenarioId(numPeople, args.numPersonPartitions,
      numLocations, args.numLocationPartitions);
  }

  diseaseModel = new DiseaseModel(args.diseasePath, personAttributes);
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

  CkPrintf("Locations Attributes:\n");
  for (int i = 0; i < locationAttributes.size(); i++) {
    CkPrintf("(%d) %s: default: %lf, type: %d\n",
        i, locationAttributes.getName(i).c_str(),
        locationAttributes.getDefaultValueAsDouble(i),
        locationAttributes.getDataType(i));
  }
#endif
}

void Scenario::applyInterventions(int day, Id newDailyInfections) {
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

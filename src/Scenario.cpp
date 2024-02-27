/* Copyright 2020-2024 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "Scenario.h"
#include "Types.h"
#include "Defs.h"
#include "DiseaseModel.h"
#include "contact_model/ContactModel.h"
#include "readers/Preprocess.h"
#include "intervention_model/Intervention.h"
#include "intervention_model/VaccinationIntervention.h"
#include "intervention_model/SelfIsolationIntervention.h"
#include "intervention_model/SchoolClosureIntervention.h"

#include "charm++.h"

Partitioner::Partitioner(std::string scenarioPath,
    PartitionId numPersonPartitions,
    PartitionId numLocationPartitions,
    loimos::proto::CSVDefinition *personMetadata,
    loimos::proto::CSVDefinition *locationMetadata) {
  Id firstLocationIdx = getFirstIndex(locationMetadata, scenarioPath + "locations.csv");
  Id firstPersonIdx = getFirstIndex(personMetadata, scenarioPath + "people.csv");

  Id numPeople = personMetadata->num_rows();
  Id numLocations = locationMetadata->num_rows();

  //if (0 == CkMyNode()) {
  //  CkPrintf("People offsets:\n");
  //}
  setPartitionOffsets(numPersonPartitions, firstPersonIdx, numPeople,
    personMetadata, &personPartitionOffsets);
  //if (0 == CkMyNode()) {
  //  CkPrintf("Location offsets:\n");
  //}
  setPartitionOffsets(numLocationPartitions, firstLocationIdx, numLocations,
    locationMetadata, &locationPartitionOffsets);

// #if ENABLE_DEBUG >= DEBUG_PER_CHARE
//   if (0 == CkMyNode()) {
//     for (int i = 0; i < personPartitionOffsets.size(); ++i) {
//       CkPrintf("  Person Offset %d: " ID_PRINT_TYPE "\n",
//         i, personPartitionOffsets[i]);
//     }
//     for (int i = 0; i < locationPartitionOffsets.size(); ++i) {
//       CkPrintf("  Location Offset %d: " ID_PRINT_TYPE "\n",
//         i, locationPartitionOffsets[i]);
//     }
//   }
// #endif
}

Partitioner::Partitioner(PartitionId numPersonPartitions,
    PartitionId numLocationPartitions, Id numPeople_,
    Id numLocations_) :
    numPeople(numPeople_), numLocations(numLocations_) {
  setPartitionOffsets(numPersonPartitions, 0, numPeople,
    &personPartitionOffsets);
  setPartitionOffsets(numLocationPartitions, 0, numLocations,
    &locationPartitionOffsets);
}

void Partitioner::setPartitionOffsets(PartitionId numPartitions,
    Id firstIndex, Id numObjects, loimos::proto::CSVDefinition *metadata,
    std::vector<Id> *partitionOffsets) {
  partitionOffsets->reserve(numPartitions);
  Id lastIndex = firstIndex + numObjects;

  if (0 < metadata->partition_offsets_size()) {
    PartitionId numOffsets = metadata->partition_offsets_size();
    for (PartitionId i = 0; i < numPartitions; ++i) {
      PartitionId offsetIdx = getFirstIndex(i, numOffsets, numPartitions, 0);
      Id offset = metadata->partition_offsets(offsetIdx);
      partitionOffsets->emplace_back(offset);

#ifdef ENABLE_DEBUG
      if (outOfBounds(firstIndex, lastIndex, offset)) {
        CkAbort("Error: Offset " ID_PRINT_TYPE " outside of valid range [0,"
          ID_PRINT_TYPE")\n", offset, numObjects);

      // Offsets should be sorted so we can do a binary search later
      } else if (0 != i && partitionOffsets->at(i - 1) > offset) {
        CkAbort("Error: Offset " ID_PRINT_TYPE " (%d-th offset) for chare "
        PARTITION_ID_PRINT_TYPE" out of order\n", offset, offsetIdx, i);
      }
      // else if (0 == CkMyNode()) {
      //   CkPrintf("  Chare %d: offset " ID_PRINT_TYPE " (provided)\n",
      //       i, offset);
      // }
#endif  // ENABLE_DEBUG
    }

  } else {
    Partitioner::setPartitionOffsets(numPartitions, firstIndex, numObjects,
      partitionOffsets);
  }
}
  
// If no offsets are provided, try to put about the same number of objects
// in each partition (i.e. use the old partitioning scheme)
void Partitioner::setPartitionOffsets(PartitionId numPartitions,
    Id firstIndex, Id numObjects, std::vector<Id> *partitionOffsets) {
  for (PartitionId p = 0; p < numPartitions; ++p) {
    Id offset = getFirstIndex(p, numObjects,
        numPartitions, firstIndex);
    partitionOffsets->emplace_back(offset);

#ifdef ENABLE_DEBUG
    if (outOfBounds(firstIndex, lastIndex, offset)) {
    CkAbort("Error: Offset " ID_PRINT_TYPE " outside of valid range [0,"
        ID_PRINT_TYPE")\n", offset, numObjects);
    }
    // else if (0 == CkMyNode()) {
    //   CkPrintf("  Chare %d: offset " ID_PRINT_TYPE " (default)\n",
    //       p, offset);
    // }
#endif  // ENABLE_DEBUG
  }
}

Id Partitioner::getLocalLocationIndex(Id globalIndex, PartitionId PartitionId) const {
  return getLocalIndex(globalIndex, PartitionId, locationPartitionOffsets);
}

Id Partitioner::getGlobalLocationIndex(Id localIndex, PartitionId PartitionId) const {
  return getGlobalIndex(localIndex, PartitionId, locationPartitionOffsets);
}

CacheOffset Partitioner::getPersonCacheIndex(Id globalIndex) const {
  return getLocalIndex(globalIndex, 0, locationPartitionOffsets);
}

PartitionId Partitioner::getLocationPartitionIndex(Id globalIndex) const {
  return getPartition(globalIndex, locationPartitionOffsets);
}

Id Partitioner::getLocationPartitionSize(PartitionId partitionIndex) const {
  return getPartitionSize(partitionIndex, numLocations, locationPartitionOffsets);
}

Id Partitioner::getLocalPersonIndex(Id globalIndex, PartitionId partitionIndex) const {
  return getLocalIndex(globalIndex, partitionIndex, personPartitionOffsets);
}

Id Partitioner::getGlobalPersonIndex(Id localIndex, PartitionId partitionIndex) const {
  return getGlobalIndex(localIndex, partitionIndex, personPartitionOffsets);
}

CacheOffset Partitioner::getLocationCacheIndex(Id globalIndex) const {
  return getLocalIndex(globalIndex, 0, personPartitionOffsets);
}

PartitionId Partitioner::getPersonPartitionIndex(Id globalIndex) const {
  return getPartition(globalIndex, personPartitionOffsets);
}

Id Partitioner::getPersonPartitionSize(PartitionId partitionIndex) const {
  return getPartitionSize(partitionIndex, numPeople, personPartitionOffsets);
}

InterventionModel::InterventionModel(std::string interventionPath,
    AttributeTable *personAttributes, AttributeTable *locationAttributes,
    const DiseaseModel &diseaseModel) {
  interventionDef = new loimos::proto::InterventionModel();
  readProtobuf(interventionPath, interventionDef);
  
  triggerFlags.resize(interventionDef->triggers_size(), false);
  
  personAttributes->readAttributes(interventionDef->person_attributes());
  locationAttributes->readAttributes(interventionDef->location_attributes());
  
  initPersonInterventions(interventionDef->person_interventions(),
      *personAttributes, diseaseModel);
  initLocationInterventions(
      interventionDef->location_interventions(),
      *locationAttributes, diseaseModel);
}

void InterventionModel::initPersonInterventions(
    const InterventionList &interventionSpecs,
    const AttributeTable &attributes, const DiseaseModel &diseaseModel) {
  for (uint i = 0; i < interventionSpecs.size(); ++i) {
    const loimos::proto::InterventionModel::Intervention &spec =
      interventionSpecs[i];

    if (spec.has_self_isolation()) {
      personInterventions.emplace_back(new SelfIsolationIntervention(
        spec, *diseaseModel.model, attributes));

    } else if (spec.has_vaccination()) {
      personInterventions.emplace_back(new VaccinationIntervention(
        spec, *diseaseModel.model, attributes));
    }
  }
}

void InterventionModel::initLocationInterventions(
    const InterventionList &interventionSpecs,
    const AttributeTable &attributes,
    const DiseaseModel &diseaseModel) {
  for (uint i = 0; i < interventionSpecs.size(); ++i) {
    const loimos::proto::InterventionModel::Intervention &spec =
      interventionSpecs[i];

    if (spec.has_school_closures()) {
      locationInterventions.emplace_back(new SchoolClosureIntervention(
        spec, *diseaseModel.model, attributes));
    }
  }
}
  
void SpecialAttributes::init(const AttributeTable &personAttributes,
    const AttributeTable &locationAttributes) {
  ageIndex = personAttributes.getAttributeIndex("age");
  susceptibilityIndex = personAttributes.getAttributeIndex("susceptibility");
  infectivityIndex = personAttributes.getAttributeIndex("infectivity");

  maxSimVisitsIndex = locationAttributes.getAttributeIndex("max_simultaneous_visits");
}

Scenario::Scenario(Arguments *args) : numDays(args->numDays),
    numDaysWithDistinctVisits(args->numDaysWithDistinctVisits) {
  diseaseModel = new DiseaseModel(args->diseasePath);

  personDef = NULL;
  locationDef = NULL;
  if (!args->isOnTheFlyRun) {
    // Handle people...
    personDef = new loimos::proto::CSVDefinition;
    readProtobuf(args->scenarioPath + "people.textproto", personDef);
    numPeople = personDef->num_rows();

    // ...locations...
    locationDef = new loimos::proto::CSVDefinition;
    readProtobuf(args->scenarioPath + "locations.textproto", locationDef);
    numLocations = locationDef->num_rows();

    // ...and visits
    visitDef = new loimos::proto::CSVDefinition;
    readProtobuf(args->scenarioPath + "visits.textproto", visitDef);

    personAttributes.readAttributes(personDef->fields());
    locationAttributes.readAttributes(locationDef->fields());

    partitioner = new Partitioner(args->scenarioPath, args->numPersonPartitions,
      args->numLocationPartitions, personDef, locationDef);

    if (0 == CkMyNode()) {
      buildCache(args->scenarioPath, numPeople, partitioner->personPartitionOffsets,
        numLocations, partitioner->locationPartitionOffsets, numDaysWithDistinctVisits);
    }   

  } else {
    onTheFly = &args->onTheFly;
    numPeople = onTheFly->peopleGrid.area();
    numLocations = onTheFly->locationGrid.area();

    partitioner = new Partitioner(args->numPersonPartitions,
        args->numLocationPartitions, numPeople, numLocations);
  }

  if (args->hasIntervention) {
    interventionModel = new InterventionModel(args->interventionPath,
      &personAttributes, &locationAttributes, *diseaseModel);
  }

  specialAttributes.init(personAttributes, locationAttributes);
  contactModel = createContactModel(args->contactModelType);

#if ENABLE_DEBUG >= DEBUG_BASIC
  CkPrintf("Person Attributes:\n");
  for (int i = 0; i < diseaseModel->personAttributes.size(); i++) {
    CkPrintf("(%d) %s: default: %lf, type: %d\n",
        i, diseaseModel->personAttributes.getName(i).c_str(),
        diseaseModel->personAttributes.getDefaultValueAsDouble(i),
        diseaseModel->personAttributes.getDataType(i));
  }

  CkPrintf("Locations Attributes:\n");
  for (int i = 0; i < diseaseModel->locationAttributes.size(); i++) {
    CkPrintf("(%d) %s: default: %lf, type: %d\n",
        i, diseaseModel->locationAttributes.getName(i).c_str(),
        diseaseModel->locationAttributes.getDefaultValueAsDouble(i),
        diseaseModel->locationAttributes.getDataType(i));
  }
#endif
}

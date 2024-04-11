/* Copyright 2020-2024 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "InterventionModel.h"
#include "Intervention.h"
#include "VaccinationIntervention.h"
#include "SelfIsolationIntervention.h"
#include "SchoolClosureIntervention.h"
#include "../Extern.h"
#include "../DiseaseModel.h"
#include "../readers/AttributeTable.h"

InterventionModel::InterventionModel() : interventionDef(NULL) {}

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

const Intervention<Person> & InterventionModel::getPersonIntervention(int index)
  const {
  return *personInterventions[index];
}

const Intervention<Location> & InterventionModel::getLocationIntervention(int index)
  const {
  return *locationInterventions[index];
}

int InterventionModel::getNumPersonInterventions() const {
  return static_cast<int>(personInterventions.size());
}

int InterventionModel::getNumLocationInterventions() const {
  return static_cast<int>(locationInterventions.size());
}

void InterventionModel::applyInterventions(int day, Id newDailyInfections,
    Id numPeople) {
  toggleInterventions(day, newDailyInfections, numPeople);

  for (uint i = 0; i < personInterventions.size(); ++i) {
    if (triggerFlags[personInterventions[i]->getTriggerIndex()]) {
      peopleArray.ReceiveIntervention(i);
    }
  }
  for (uint i = 0; i < locationInterventions.size(); ++i) {
    if (triggerFlags[locationInterventions[i]->getTriggerIndex()]) {
      locationsArray.ReceiveIntervention(i);
    }
  }
}

void InterventionModel::toggleInterventions(int day, Id newDailyInfections,
    Id numPeople) {
  for (uint i = 0; i < interventionDef->triggers_size(); ++i) {
    const loimos::proto::InterventionModel::Trigger &trigger =
      interventionDef->triggers(i);

    // Trigger uses simulation day
    if (trigger.has_day()) {
      const auto &tmp = trigger.day();
      triggerFlags[i] = (!triggerFlags[i] && tmp.trigger_on() <= day)
        || (triggerFlags[i] && tmp.trigger_off() <= day);

    // Trigger uses number of new cases
    } else if (trigger.has_new_daily_cases()) {
      double infectionRate = static_cast<double>(newDailyInfections)
        / numPeople;
      const auto &tmp = trigger.new_daily_cases();
      triggerFlags[i] =
        (!triggerFlags[i] && tmp.trigger_on() <= infectionRate)
        || (triggerFlags[i] && tmp.trigger_off() <= infectionRate);
    }
  }
}

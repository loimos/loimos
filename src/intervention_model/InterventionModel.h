/* Copyright 2020-2024 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef INTERVENTION_MODEL_INTERVENTIONMODEL_H_
#define INTERVENTION_MODEL_INTERVENTIONMODEL_H_

#include "Intervention.h"
#include "../Person.h"
#include "../Location.h"
#include "../DiseaseModel.h"
#include "../readers/AttributeTable.h"

#include <vector>
#include <memory>
#include <string>

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
#endif  // INTERVENTION_MODEL_INTERVENTIONMODEL_H_

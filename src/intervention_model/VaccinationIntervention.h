 /* Copyright 2020-2023 The Loimos Project Developers.
  * See the top-level LICENSE file for details.
  *
  * SPDX-License-Identifier: MIT
  */

#ifndef INTERVENTION_MODEL_VACCINATIONINTERVENTION_H_
#define INTERVENTION_MODEL_VACCINATIONINTERVENTION_H_

#include "Intervention.h"
#include "../Person.h"
#include "../protobuf/interventions.pb.h"
#include "../protobuf/disease.pb.h"
#include "../readers/DataInterface.h"
#include "../readers/AttributeTable.h"

#include "charm++.h"

class VaccinationIntervention : public Intervention<Person> {
 private:
  double vaccinationProbability;
  double vaccinatedSusceptibility;
  int vaccinatedIndex;
  int susceptibilityIndex;

 public:
  VaccinationIntervention(
      const loimos::proto::InterventionModel::Intervention &interventionDef,
      const loimos::proto::DiseaseModel &diseaseDef,
      const AttributeTable &t);
  VaccinationIntervention() {}

  bool test(const Person &p, std::default_random_engine *generator)
      const override;
  void apply(Person *p) const override;
};

#endif  // INTERVENTION_MODEL_VACCINATIONINTERVENTION_H_

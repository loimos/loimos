 /* Copyright 2020-2023 The Loimos Project Developers.
  * See the top-level LICENSE file for details.
  *
  * SPDX-License-Identifier: MIT
  */

#ifndef INTERVENTION_MODEL_VACCINATIONINTERVENTION_H_
#define INTERVENTION_MODEL_VACCINATIONINTERVENTION_H_

#include "Intervention.h"
#include "AttributeTable.h"
#include "../Person.h"
#include "../protobuf/interventions.pb.h"
#include "../readers/DataInterface.h"

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
      const AttributeTable &t);
  VaccinationIntervention() {};

  bool test(const Person &p, std::default_random_engine *generator)
      const override;
  void apply(Person *p) const override;
};

#endif  // INTERVENTION_MODEL_VACCINATIONINTERVENTION_H_

 /* Copyright 2020-2023 The Loimos Project Developers.
  * See the top-level LICENSE file for details.
  *
  * SPDX-License-Identifier: MIT
  */

#ifndef INTERVENTION_MODEL_SELFISOLATIONINTERVENTION_H_
#define INTERVENTION_MODEL_SELFISOLATIONINTERVENTION_H_

#include "AttributeTable.h"
#include "Intervention.h"
#include "VisitFilterIntervention.h"
#include "../Person.h"
#include "../protobuf/interventions.pb.h"
#include "../protobuf/disease.pb.h"
#include "../readers/DataInterface.h"

#include "charm++.h"

class SelfIsolationIntervention : public VisitFilterIntervention<Person> {
 protected:
  int schoolIndex;
  const loimos::proto::DiseaseModel &diseaseDef;
 public:
  SelfIsolationIntervention(
      const loimos::proto::InterventionModel::Intervention &interventionDef,
      const loimos::proto::DiseaseModel &diseaseDef_,
      const AttributeTable &t) : diseaseDef(diseaseDef_),
    VisitFilterIntervention<Person>(interventionDef, diseaseDef_, t) {}
  virtual bool test(const Person &p, std::default_random_engine *generator) const {
    return diseaseDef.disease_states(p.state).symptomatic();
  }
};

#endif  // INTERVENTION_MODEL_SELFISOLATIONINTERVENTION_H_

 /* Copyright 2020-2023 The Loimos Project Developers.
  * See the top-level LICENSE file for details.
  *
  * SPDX-License-Identifier: MIT
  */

#ifndef INTERVENTION_MODEL_SCHOOLCLOSUREINTERVENTION_H_
#define INTERVENTION_MODEL_SCHOOLCLOSUREINTERVENTION_H_

#include "Intervention.h"
#include "VisitFilterIntervention.h"
#include "../Location.h"
#include "../protobuf/interventions.pb.h"
#include "../protobuf/disease.pb.h"
#include "../readers/DataInterface.h"
#include "../readers/AttributeTable.h"

class SchoolClosureIntervention : public VisitFilterIntervention<Location> {
 protected:
  int schoolIndex;
 public:
  SchoolClosureIntervention(
      const loimos::proto::InterventionModel::Intervention &interventionDef,
      const loimos::proto::DiseaseModel &diseaseDef,
      const AttributeTable &t) :
    VisitFilterIntervention<Location>(interventionDef, diseaseDef, t) {
    schoolIndex = t.getAttributeIndex("school");
  }

  bool test(const Location &p, std::default_random_engine *generator) const override {
    return p.getValue(schoolIndex).boolean;
  }
};

#endif  // INTERVENTION_MODEL_SCHOOLCLOSUREINTERVENTION_H_

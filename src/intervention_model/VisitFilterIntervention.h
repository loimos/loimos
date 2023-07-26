 /* Copyright 2020-2023 The Loimos Project Developers.
  * See the top-level LICENSE file for details.
  *
  * SPDX-License-Identifier: MIT
  */

#ifndef INTERVENTION_MODEL_VISITFILTERINTERVENTION_H_
#define INTERVENTION_MODEL_VISITFILTERINTERVENTION_H_

#include "AttributeTable.h"
#include "../protobuf/interventions.pb.h"
#include "../readers/DataInterface.h"

#include "charm++.h"
#include <functional>

template <class T = DataInterface>
class VisitFilterIntervention : public Intervention<T> {
 protected:
  VisitTest keepVisit;
 public:
  VisitFilterIntervention(
      const loimos::proto::InterventionModel::Intervention &interventionDef,
      const loimos::proto::DiseaseModel &diseaseDef,
      const AttributeTable &t) :
    Intervention<T>(interventionDef, diseaseDef, t) {
    keepVisit = [](const VisitMessage &visit) {
      return false;
    };
  }

  void apply(T *p) const override {
    p->filterVisits(this, keepVisit);
  }
  void remove(T *p) const override {
    p->restoreVisits(this);
  }  
};

#endif  // INTERVENTION_MODEL_VISITFILTERINTERVENTION_H_

 /* Copyright 2020-2023 The Loimos Project Developers.
  * See the top-level LICENSE file for details.
  *
  * SPDX-License-Identifier: MIT
  */

#ifndef INTERVENTION_MODEL_INTERVENTION_H_
#define INTERVENTION_MODEL_INTERVENTION_H_

#include "../protobuf/interventions.pb.h"
#include "../protobuf/disease.pb.h"
#include "../readers/DataInterface.h"
#include "../readers/AttributeTable.h"

#include "charm++.h"

using InterventionList = google::protobuf::RepeatedPtrField<
  loimos::proto::InterventionModel::Intervention>;

template <class T = DataInterface>
class Intervention {
 protected:
  static std::uniform_real_distribution<double> unitDistrib;
  double compliance;
  int triggerIndex;

 public:
  int getTriggerIndex() const {
    return triggerIndex;
  }
  bool willComply(const T &p, std::default_random_engine *generator) const {
    return unitDistrib(*generator) < compliance;
  }
  virtual bool test(const T &p, std::default_random_engine *generator) const {
    return false;
  }
  // Applies intervention to object
  virtual void apply(T *p) const {}
  // Undoes any previous intervention application on this object.
  // For any intervention that cannot be undone, this should have no effect.
  virtual void remove(T *p) const {}

  Intervention() {}
  Intervention(
      const loimos::proto::InterventionModel::Intervention &interventionDef,
      const loimos::proto::DiseaseModel &diseaseDef,
      const AttributeTable &t) {
    compliance = interventionDef.compliance();
    triggerIndex = interventionDef.trigger_index();
  }
};

template <class T>
std::uniform_real_distribution<double> Intervention<T>::unitDistrib(
    0.0, 1.0);

#endif  // INTERVENTION_MODEL_INTERVENTION_H_

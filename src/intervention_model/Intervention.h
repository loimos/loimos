 /* Copyright 2020-2023 The Loimos Project Developers.
  * See the top-level LICENSE file for details.
  *
  * SPDX-License-Identifier: MIT
  */

#ifndef INTERVENTION_MODEL_INTERVENTION_H_
#define INTERVENTION_MODEL_INTERVENTION_H_

#include "AttributeTable.h"
#include "../protobuf/interventions.pb.h"
#include "../readers/DataInterface.h"

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
  virtual void apply(T *p) const {}

  Intervention() {}
  Intervention(
      const loimos::proto::InterventionModel::Intervention &interventionDef,
      const AttributeTable &t) {
    compliance = interventionDef.compliance();
    triggerIndex = interventionDef.trigger_index();
  }
};

template <class T>
std::uniform_real_distribution<double> Intervention<T>::unitDistrib(
    0.0, 1.0);

#endif  // INTERVENTION_MODEL_INTERVENTION_H_

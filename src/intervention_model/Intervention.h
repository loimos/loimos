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
#include "../Location.h"

#include "charm++.h"
#include <vector>
#include <unordered_set>

using InterventionList = google::protobuf::RepeatedPtrField<
  loimos::proto::InterventionModel::Intervention>;

template <class T = DataInterface>
class Intervention {
 protected:
  static std::uniform_real_distribution<double> unitDistrib;
  double compliance;
  int triggerIndex;
  const uint interventionIndex;

 public:
  int getTriggerIndex() const {
    return triggerIndex;
  }
  bool willComply(const T &p, std::default_random_engine *generator) const {
    return unitDistrib(*generator) < compliance;
  }
  virtual bool shouldApply(const T &p, std::default_random_engine *generator) const {
    return false;
  }
  virtual bool shouldRemove(const T &p, std::default_random_engine *generator) const {
    return false;
  }
  // Applies intervention to object
  virtual void apply(T *p) const {}
  // Undoes any previous intervention application on this object.
  // For any intervention that cannot be undone, this should have no effect.
  virtual void remove(T *p) const {}

  // Applies the intervention to or removes it from all objects on a chare
  virtual void apply(std::vector<T> *data, bool isActive,
      std::unordered_set<Id> *applied, std::unordered_set<Id> *removed) const {
    if (isActive) {
      for (T &d : *data) {
        if (d.willComply(interventionIndex)
            && shouldApply(d, d.getGenerator())) {
          apply(&d);
        } else if (d.isActive(interventionIndex)
            && shouldRemove(d, d.getGenerator())) {
          remove(&d);
      }
    }

    } else {
      for (T &d : *data) {
        if (d.isActive(interventionIndex)) {
          remove(&d);
        }
      }
    }
  }

  virtual void apply(std::vector<Location> *data,
    const std::unordered_set<Id> &applied,
    const std::unordered_set<Id> &removed) const {}

  Intervention(
      const loimos::proto::InterventionModel::Intervention &interventionDef,
      const loimos::proto::DiseaseModel &diseaseDef,
      const AttributeTable &t, uint _interventionIndex) :
      interventionIndex(_interventionIndex) {
    compliance = interventionDef.compliance();
    triggerIndex = interventionDef.trigger_index();
  }
};

template <class T>
std::uniform_real_distribution<double> Intervention<T>::unitDistrib(
    0.0, 1.0);

#endif  // INTERVENTION_MODEL_INTERVENTION_H_

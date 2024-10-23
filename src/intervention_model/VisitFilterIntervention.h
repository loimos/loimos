 /* Copyright 2020-2023 The Loimos Project Developers.
  * See the top-level LICENSE file for details.
  *
  * SPDX-License-Identifier: MIT
  */

#ifndef INTERVENTION_MODEL_VISITFILTERINTERVENTION_H_
#define INTERVENTION_MODEL_VISITFILTERINTERVENTION_H_

#include "../protobuf/interventions.pb.h"
#include "../readers/DataInterface.h"
#include "../readers/AttributeTable.h"

#include "charm++.h"
#include <functional>
#include <unordered_set>

template <class T = DataInterface>
class VisitFilterIntervention : public Intervention<T> {
 protected:
  VisitTest keepVisit;
 public:
  VisitFilterIntervention(
      const loimos::proto::InterventionModel::Intervention &interventionDef,
      const loimos::proto::DiseaseModel &diseaseDef,
      const AttributeTable &t, uint index) :
    Intervention<T>(interventionDef, diseaseDef, t, index) {
    keepVisit = [](const VisitMessage &visit) {
      return false;
    };
  }

  void apply(T *d) const override {
    d->filterVisits(this, keepVisit);
  }

  void remove(T *d) const override {
    d->restoreVisits(this);
  }

  void apply(std::vector<Person> *data, bool isActive,
    std::unordered_set<Id> *applied,
    std::unordered_set<Id> *removed) const override;
  
  virtual void apply(std::vector<Location> *data,
    const std::unordered_set<Id> &applied,
    const std::unordered_set<Id> &removed) const override;
};

template <>
void VisitFilterIntervention<Person>::apply(std::vector<Person> *data, bool isActive,
    std::unordered_set<Id> *applied,
    std::unordered_set<Id> *removed) const {
  if (isActive) {
    for (T &d : *data) {
      if (d.willComply(interventionIndex)
          && shouldApply(d, d.getGenerator())) {
        applied->add(d.getUniqueId());
      } else if (d.isActive(interventionIndex)
        && shouldRemove(d, d.getGenerator())) {
        removed->add(d.getUniqueId());
    }
  }

  } else {
    for (T &d : *data) {
      if (d.isActive(interventionIndex)) {
        removed->add(d.getUniqueId());
      }
    }
  }
}

template <>
void VisitFilterIntervention<Person>::apply(std::vector<Location> *data,
    const std::unordered_set<Id> &applied,
    const std::unordered_set<Id> &removed) const {
  VisitTest notInApplied = [&applied](const VisitMessage &visit) {
    return applied.find(visit.getPersonId()) == applied.end();
  };
  for (Location &d : *data) {
    if (applied.find(d.getUniqueId()) != applied.end()) {
      d.filterVisits(this, notInApplied);
    } else if (removed.find(d.getUniqueId()) != removed.end()) {
      d.restoreVisits(this);
    }
  }
}

#endif  // INTERVENTION_MODEL_VISITFILTERINTERVENTION_H_

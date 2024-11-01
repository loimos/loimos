 /* Copyright 2020-2023 The Loimos Project Developers.
  * See the top-level LICENSE file for details.
  *
  * SPDX-License-Identifier: MIT
  */

#ifndef INTERVENTION_MODEL_VISITFILTERINTERVENTION_H_
#define INTERVENTION_MODEL_VISITFILTERINTERVENTION_H_

#include "../protobuf/interventions.pb.h"
#include "Intervention.h"
#include "../readers/DataInterface.h"
#include "../readers/AttributeTable.h"

#include "charm++.h"
#include <functional>
#include <unordered_set>
#include <type_traits>

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

  // This just exists to dispatch to the other two implementations
  void apply(std::vector<T> *data, bool isActive,
      std::unordered_set<Id> *applied,
      std::unordered_set<Id> *removed) const override {
    apply(data, isActive, applied, removed, std::is_same<T, Person>());
  }

  void apply(std::vector<T> *data, bool isActive,
      std::unordered_set<Id> *applied,
      std::unordered_set<Id> *removed,
      std::false_type) const {
    Intervention<T>::apply(data, isActive, applied, removed);
  }

  void apply(std::vector<T> *data, bool isActive,
      std::unordered_set<Id> *applied,
      std::unordered_set<Id> *removed,
      std::true_type) const {
    if (isActive) {
      for (T &d : *data) {
        if (d.willComply(this->interventionIndex)
            && this->shouldApply(d, d.getGenerator())) {
          applied->emplace(d.getUniqueId());
        } else if (d.isActive(this->interventionIndex)
          && this->shouldRemove(d, d.getGenerator())) {
          removed->emplace(d.getUniqueId());
      }
    }

    } else {
      for (T &d : *data) {
        if (d.isActive(this->interventionIndex)) {
          removed->emplace(d.getUniqueId());
        }
      }
    }
  }

  // This just exists to dispatch to the other two implementations
  void apply(std::vector<Location> *data,
      const std::unordered_set<Id> &applied,
      const std::unordered_set<Id> &removed) const {
    apply(data, applied, removed, std::is_same<T, Person>());
  }

  void apply(std::vector<Location> *data,
      const std::unordered_set<Id> &applied,
      const std::unordered_set<Id> &removed,
      std::false_type) const {}

  void apply(std::vector<Location> *data,
      const std::unordered_set<Id> &applied,
      const std::unordered_set<Id> &removed,
      std::true_type) const {
    VisitTest notInApplied = [&](const VisitMessage &visit) {
      return applied.find(visit.personIdx) == applied.end();
    };
    for (Location &d : *data) {
      if (applied.find(d.getUniqueId()) != applied.end()) {
        d.filterVisits(this, notInApplied);
      } else if (removed.find(d.getUniqueId()) != removed.end()) {
        d.restoreVisits(this);
      }
    }
  }
};


#endif  // INTERVENTION_MODEL_VISITFILTERINTERVENTION_H_

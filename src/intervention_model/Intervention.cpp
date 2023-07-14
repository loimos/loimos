 /* Copyright 2020-2023 The Loimos Project Developers.
  * See the top-level LICENSE file for details.
  *
  * SPDX-License-Identifier: MIT
  */

#include "Intervention.h"
#include "../protobuf/interventions.pb.h"
#include "../readers/DataInterface.h"

#include <vector>


std::uniform_real_distribution<double> Intervention::unitDistrib(0.0, 1.0);

Intervention::Intervention(
    const loimos::proto::InterventionModel::Intervention &interventionDef,
    const AttributeTable &t) {
  compliance = interventionDef.compliance();
}

bool Intervention::willComply(const DataInterface &p,
    std::default_random_engine *generator) const {
  return unitDistrib(*generator) < compliance;
}

bool Intervention::test(const DataInterface &p,
    std::default_random_engine *generator) const {
  return false;
}

void Intervention::apply(DataInterface *p) const {}


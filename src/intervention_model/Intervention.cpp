 /* Copyright 2020-2023 The Loimos Project Developers.
  * See the top-level LICENSE file for details.
  *
  * SPDX-License-Identifier: MIT
  */

#include "Intervention.h"
#include "../protobuf/interventions.pb.h"
#include "../readers/DataInterface.h"

#include <vector>

bool Intervention::test(const DataInterface &p,
    std::default_random_engine *generator) const {
  return false;
}

void Intervention::apply(DataInterface *p) const {}

void Intervention::pup(PUP::er &p) {}

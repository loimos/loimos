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

class Intervention : public PUP::able {
 protected:
  static std::uniform_real_distribution<double> unitDistrib;
  double compliance;
 public:
  bool willComply(const DataInterface &p,
    std::default_random_engine *generator) const;
  virtual bool test(const DataInterface &p,
      std::default_random_engine *generator) const;
  virtual void apply(DataInterface *p) const;
  virtual void pup(PUP::er &p);  // NOLINT(runtime/references)

  PUPable_decl(Intervention);
  Intervention() {}
  Intervention(
    const loimos::proto::InterventionModel::Intervention &interventionDef,
    const AttributeTable &t);
  explicit Intervention(CkMigrateMessage *m) :
    PUP::able(m) {}  // NOLINT(runtime/references)
};

#endif  // INTERVENTION_MODEL_INTERVENTION_H_

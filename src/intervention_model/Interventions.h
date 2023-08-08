 /* Copyright 2020-2023 The Loimos Project Developers.
  * See the top-level LICENSE file for details.
  *
  * SPDX-License-Identifier: MIT
  */

#ifndef INTERVENTION_MODEL_INTERVENTIONS_H_
#define INTERVENTION_MODEL_INTERVENTIONS_H_

#include "AttributeTable.h"
#include "../protobuf/interventions.pb.h"
#include "../readers/DataInterface.h"

#include "charm++.h"

class Intervention : public PUP::able {
 public:
  virtual bool test(const DataInterface &p,
      std::default_random_engine *generator) const;
  virtual void apply(DataInterface *p) const;
  virtual void pup(PUP::er &p);  // NOLINT(runtime/references)

  PUPable_decl(Intervention);
  Intervention() {}
  explicit Intervention(CkMigrateMessage *m) :
    PUP::able(m) {}  // NOLINT(runtime/references)
};

class VaccinationIntervention : public Intervention {
 public:
  double vaccinationProbability;
  double vaccinatedSusceptibility;
  int vaccinatedIndex;
  int susceptibilityIndex;

  bool test(const DataInterface &p, std::default_random_engine *generator)
    const override;
  void apply(DataInterface *p) const override;
  void pup(PUP::er &p) override;  // NOLINT(runtime/references)
  void identify();
  VaccinationIntervention(
      const loimos::proto::InterventionModel::Intervention &interventionDef,
      const AttributeTable &t);
  PUPable_decl(VaccinationIntervention);
  VaccinationIntervention();
  explicit VaccinationIntervention(CkMigrateMessage *m) : Intervention(m) {}
};
#endif  // INTERVENTION_MODEL_INTERVENTIONS_H_

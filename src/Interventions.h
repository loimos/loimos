 /* Copyright 2020-2023 The Loimos Project Developers.
  * See the top-level LICENSE file for details.
  *
  * SPDX-License-Identifier: MIT
  */

#ifndef INTERVENTIONS_H_
#define INTERVENTIONS_H_

#include "AttributeTable.h"
#include "readers/DataInterface.h"
#include "readers/interventions.pb.h"

#include "charm++.h"

class BaseIntervention : public PUP::able {
 public:
  virtual bool test(const DataInterface &p,
      std::default_random_engine *generator) const;
  virtual void apply(DataInterface *p) const;
  virtual void pup(PUP::er &p);  // NOLINT(runtime/references)

  PUPable_decl(BaseIntervention);
  BaseIntervention() {}
  explicit BaseIntervention(CkMigrateMessage *m) :
    PUP::able(m) {}  // NOLINT(runtime/references)
};

class VaccinationIntervention : public BaseIntervention {
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
  explicit VaccinationIntervention(CkMigrateMessage *m) : BaseIntervention(m) {}
};
#endif  // INTERVENTIONS_H_

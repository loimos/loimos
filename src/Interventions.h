 /* Copyright 2020-2023 The Loimos Project Developers.
  * See the top-level LICENSE file for details.
  *
  * SPDX-License-Identifier: MIT
  */

#ifndef INTERVENTIONS_H_
#define INTERVENTIONS_H_

#include "readers/DataInterface.h"
#include "AttributeTable.h"

#include "charm++.h"

class BaseIntervention : public PUP::able {
 public:
  virtual bool test(const DataInterface &p, std::default_random_engine *generator);
  virtual void apply(DataInterface *p);
  virtual void pup(PUP::er &p);  // NOLINT(runtime/references)

  PUPable_decl(BaseIntervention);
  explicit BaseIntervention() {}
  explicit BaseIntervention(CkMigrateMessage *m) :
    PUP::able(m) {}  // NOLINT(runtime/references)
};

class VaccinationIntervention : public BaseIntervention {
 public:
  double vaccinationProbability;
  int riskIndex;
  int vaccinatedIndex;
  int probIndex;
  bool test(const DataInterface &p, std::default_random_engine *generator) override;
  void apply(DataInterface *p) override;
  void pup(PUP::er &p) override;  // NOLINT(runtime/references)
  void identify();
  explicit VaccinationIntervention(const AttributeTable &t);
  PUPable_decl(VaccinationIntervention);
  VaccinationIntervention();
  explicit VaccinationIntervention(CkMigrateMessage *m) : BaseIntervention(m) {}
};
#endif  // INTERVENTIONS_H_

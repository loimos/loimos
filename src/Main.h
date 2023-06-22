/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "charm++.h"
#include "Interventions.h"

#include <vector>
#include <string>
#include <memory>

class Main : public CBase_Main {
  Main_SDAG_CODE
  int day;
  std::string pathToOutput;
  std::vector<int> accumulated;
  std::vector<int> initialInfections;
  DiseaseModel* diseaseModel;
  int chareCount;
  int createdCount;

 public:
  explicit Main(CkArgMsg* msg);
  void CharesCreated();
  void SeedInfections();
  std::vector<std::shared_ptr<BaseIntervention>> GetInterventions();
  void SaveStats(int *data);
};

#endif  // MAIN_H_

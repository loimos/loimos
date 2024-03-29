/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "charm++.h"

#include <vector>
#include <string>
#include <memory>

class Main : public CBase_Main {
  Main_SDAG_CODE
  int day;
  int seed;
  std::vector<int> accumulated;
  std::vector<int> initialInfections;
  DiseaseModel* diseaseModel;
  PartitionId chareCount;
  PartitionId createdCount;
  Id lastInfectiousCount;

 public:
  explicit Main(CkArgMsg* msg);
  void CharesCreated();
  void SeedInfections();
  void SaveStats(Id *data);
};

#endif  // MAIN_H_

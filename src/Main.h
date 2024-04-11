/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "charm++.h"
#include "Scenario.h"
#include "Types.h"

#include <vector>
#include <string>
#include <memory>

class Main : public CBase_Main {
  Main_SDAG_CODE
  int day;
  std::vector<int> accumulated;
  std::vector<int> initialInfections;
  PartitionId chareCount;
  PartitionId createdCount;
  Id lastInfectiousCount;

  Scenario *scenario;
  Profile profile;

 public:
  explicit Main(CkArgMsg* msg);
  void CharesCreated();
  void SeedInfections();
  void SaveStats(Id *data);
};

#endif  // MAIN_H_

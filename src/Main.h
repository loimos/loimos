/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __MAIN_H__
#define __MAIN_H__

#include <vector>
#include <string>
#include "charm++.h"

class Main : public CBase_Main {
  Main_SDAG_CODE
  int day;
  std::string pathToOutput;
  std::vector<int> accumulated;
  DiseaseModel* diseaseModel;
  int chareCount;
  int createdCount;

  public:
    Main(CkArgMsg* msg);
    #ifdef USE_HYPERCOMM
    void CharesCreated();
    #endif
    void SaveStats(int *data);
};

#endif // __MAIN_H__

/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __MAIN_H__
#define __MAIN_H__

#include <vector>
#include "charm++.h"

class Main : public CBase_Main {
  Main_SDAG_CODE
  int day;
  std::vector<int> accumulated;
  DiseaseModel* diseaseModel;

  public:
    Main(CkArgMsg* msg);
};

#endif // __MAIN_H__

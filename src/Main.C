/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "Main.h"
#include "People.h"
#include "Locations.h"
#include "DiseaseModel.h"
#include "data/Preprocess.h"

#include <tuple>

/* readonly */ CProxy_Main mainProxy;
/* readonly */ CProxy_People peopleArray;
/* readonly */ CProxy_Locations locationsArray;
/* readonly */ CProxy_DiseaseModel globDiseaseModel;
/* readonly */ int numPeople;
/* readonly */ int numLocations;
/* readonly */ int numPeoplePartitions;
/* readonly */ int numLocationPartitions;
/* readonly */ std::string scenarioPath;
/* readonly */ std::string scenarioId;
/* readonly */ int numDays;
/* readonly */ int firstPersonIdx;
/* readonly */ int firstLocationIdx;

Main::Main(CkArgMsg* msg) {
  // parsing command line arguments
  if(msg->argc < 7){
    CkPrintf("Error, usage %s <people> <locations> <people subsets> <location subsets> <days> <scenario_folder>\n", msg->argv[0]);
    CkExit();
  }
  numPeople = atoi(msg->argv[1]);
  numLocations = atoi(msg->argv[2]);
  numPeoplePartitions = atoi(msg->argv[3]);
  numLocationPartitions = atoi(msg->argv[4]);
  numDays = atoi(msg->argv[5]);
  scenarioPath = std::string(msg->argv[6]);

  // setup main proxy
  CkPrintf("Running Loimos on %d PEs with %d people, %d locations, %d people subsets, %d location subsets, and %d days\n", CkNumPes(), numPeople, numLocations, numPeoplePartitions, numLocationPartitions, numDays);
  mainProxy = thisProxy;

  // Create data caches.
  std::tie(firstPersonIdx, firstLocationIdx, scenarioId) = build_cache(scenarioPath, numPeople, numPeoplePartitions, numLocations, numLocationPartitions, numDays);

  // Instantiate DiseaseModel nodegroup (One for each physical processor).
  CkPrintf("Loading diseaseModel.\n");
  globDiseaseModel = CProxy_DiseaseModel::ckNew();
  diseaseModel = globDiseaseModel.ckLocalBranch();
  accumulated.resize(diseaseModel->getNumberOfStates(), 0);
  delete msg;

  // creating chare arrays
  CkPrintf("Loading others.\n");
  peopleArray = CProxy_People::ckNew(numPeoplePartitions);
  locationsArray = CProxy_Locations::ckNew(numLocationPartitions);

  // run
  CkPrintf("Running.\n");
  mainProxy.run();
}

#include "loimos.def.h"

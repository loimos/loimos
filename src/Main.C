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
#include "readers/Preprocess.h"
#include "readers/DataLoader.h"

#include <tuple>

/* readonly */ CProxy_Main mainProxy;
/* readonly */ CProxy_People peopleArray;
/* readonly */ CProxy_Locations locationsArray;
/* readonly */ CProxy_DiseaseModel globDiseaseModel;
/* readonly */ CProxy_DataLoader dataLoaders;
/* readonly */ int numPeople;
/* readonly */ int numLocations;
/* readonly */ int numPeoplePartitions;
/* readonly */ int numLocationPartitions;
/* readonly */ int numDays;
/* readonly */ bool syntheticRun;
/* readonly */ std::string scenarioPath;
/* readonly */ std::string scenarioId;
/* readonly */ int firstPersonIdx;
/* readonly */ int firstLocationIdx;
/* readonly */ double simulationStartTime;

Main::Main(CkArgMsg* msg) {
  // parsing command line arguments
  if(msg->argc < 7){
    CkPrintf("Error, usage %s <people> <locations> <people subsets> <location subsets> <days> <disease_model_path> <scenario_folder (optional)>\n", msg->argv[0]);
    CkExit();
  }
  numPeople = atoi(msg->argv[1]);
  numLocations = atoi(msg->argv[2]);
  numPeoplePartitions = atoi(msg->argv[3]);
  numLocationPartitions = atoi(msg->argv[4]);
  numDays = atoi(msg->argv[5]);
  std::string pathToDiseaseModel = std::string(msg->argv[6]);

  // Handle both real data runs or runs using synthetic populations.
  if(msg->argc == 8) {
    syntheticRun = false;
    
    // Create data caches.
    scenarioPath = std::string(msg->argv[7]);
    std::tie(firstPersonIdx, firstLocationIdx, scenarioId) = buildCache(scenarioPath, numPeople, numPeoplePartitions, numLocations, numLocationPartitions, numDays);
  } else {
    syntheticRun = true;
    firstPersonIdx = 0;
    firstLocationIdx = 0;
  }

  // setup main proxy
  CkPrintf("Running Loimos on %d PEs with %d people, %d locations, %d people subsets, %d location subsets, and %d days\n", CkNumPes(), numPeople, numLocations, numPeoplePartitions, numLocationPartitions, numDays);
  mainProxy = thisProxy;


  // Instantiate DiseaseModel nodegroup (One for each physical processor).
  CkPrintf("Loading diseaseModel.\n");
  globDiseaseModel = CProxy_DiseaseModel::ckNew(pathToDiseaseModel);
  diseaseModel = globDiseaseModel.ckLocalBranch();
  accumulated.resize(diseaseModel->getNumberOfStates(), 0);
  delete msg;

  // creating chare arrays
  CkPrintf("Loading people and locations.\n");
  peopleArray = CProxy_People::ckNew(numPeoplePartitions);
  locationsArray = CProxy_Locations::ckNew(numLocationPartitions);
  dataLoaders = CProxy_DataLoader::ckNew(
    // Round up loaders for both.
    (numPeoplePartitions + LOADING_CHARES_PER_CHARE - 1) / LOADING_CHARES_PER_CHARE +
    (numLocationPartitions + LOADING_CHARES_PER_CHARE - 1) / LOADING_CHARES_PER_CHARE
  ); 

  // run
  CkPrintf("Running.\n");
  simulationStartTime = CkWallTimer();
  mainProxy.run();
}

#include "loimos.def.h"

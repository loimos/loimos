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
#include "contact_model/ContactModel.h"
#include "readers/Preprocess.h"

#include <tuple>

/* readonly */ CProxy_Main mainProxy;
/* readonly */ CProxy_People peopleArray;
/* readonly */ CProxy_Locations locationsArray;
/* readonly */ CProxy_DiseaseModel globDiseaseModel;
/* readonly */ int numPeople;
/* readonly */ int numLocations;
/* readonly */ int numPeoplePartitions;
/* readonly */ int numLocationPartitions;
/* readonly */ int numDays;
/* readonly */ bool syntheticRun;
/* readonly */ int contactModelType;
/* readonly */ std::string scenarioPath;
/* readonly */ std::string scenarioId;
/* readonly */ int firstPersonIdx;
/* readonly */ int firstLocationIdx;
/* readonly */ double simulationStartTime;

// For synthetic run.
/* readonly */ int synPeopleGridWidth;
/* readonly */ int synPeopleGridHeight;
/* readonly */ int synLocationGridWidth;
/* readonly */ int synLocationGridHeight;
/* readonly */ int averageDegreeOfVisit;


Main::Main(CkArgMsg* msg) {
  // parsing command line arguments
  if(msg->argc < 7){
    CkPrintf("Error, usage %s <people> <locations> <people subsets> <location subsets> <days> <disease_model_path> <scenario_folder (optional)>\n", msg->argv[0]);
    CkExit();
  }
  syntheticRun = atoi(msg->argv[1]) == 1;
  int baseRunInfo = 0;
  if (syntheticRun) {
    // Get number of people.
    synPeopleGridWidth = atoi(msg->argv[2]);
    synPeopleGridHeight = atoi(msg->argv[3]);
    numPeople = synPeopleGridWidth * synPeopleGridHeight;
    // Location data
    synLocationGridWidth = atoi(msg->argv[4]);
    synLocationGridHeight = atoi(msg->argv[5]);
    numLocations = synLocationGridWidth * synLocationGridHeight;
    assert(synPeopleGridWidth >= synLocationGridWidth);
    assert(synPeopleGridHeight >= synLocationGridHeight);

    // Edge degree.
    averageDegreeOfVisit = atoi(msg->argv[6]);
    baseRunInfo = 6;
  } else {
    numPeople = atoi(msg->argv[2]);
    numLocations = atoi(msg->argv[3]);
    baseRunInfo = 3;
  }
  
  numPeoplePartitions = atoi(msg->argv[baseRunInfo + 1]);
  numLocationPartitions = atoi(msg->argv[baseRunInfo + 2]);
  numDays = atoi(msg->argv[baseRunInfo + 3]);
  std::string pathToDiseaseModel = std::string(msg->argv[baseRunInfo + 4]);

  // Handle both real data runs or runs using synthetic populations.
  if(syntheticRun) {
    CkPrintf("Synthetic run with (%d, %d) person grid and (%d, %d) location grid. Average degree of %d\n", synPeopleGridWidth, synPeopleGridHeight, synLocationGridWidth, synLocationGridHeight, averageDegreeOfVisit);
    firstPersonIdx = 0;
    firstLocationIdx = 0;
  } else {    
    // Create data caches.
    scenarioPath = std::string(msg->argv[baseRunInfo + 5]);
    std::tie(firstPersonIdx, firstLocationIdx, scenarioId) = buildCache(scenarioPath, numPeople, numPeoplePartitions, numLocations, numLocationPartitions, numDays);
  }

  // Detemine which contact modle to use
  contactModelType = (int) ContactModelType::constant_probability;
  if (msg->argc == baseRunInfo + 7) {
    std::string tmp = std::string(msg->argv[baseRunInfo + 6]);
    // We can just use a flag for now in the CLI, since we only have two
    // models and that's easier to parse, but we may eventually have more,
    // which is why we use an enum to actually hold the model value
    if ("-m" == tmp or "--min-max-alpha" == tmp) {
      contactModelType = (int) ContactModelType::min_max_alpha;
    }
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

  // run
  CkPrintf("Running.\n");
  simulationStartTime = CkWallTimer();
  mainProxy.run();
}

#include "loimos.def.h"

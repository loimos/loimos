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
#include "schedule/Schedule.h"

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
/* readonly */ int contactModelType;
/* readonly */ int scheduleType;
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
  if(msg->argc >= 8) {
    scheduleType = (int) ScheduleType::filedSchedule;
    
    // Create data caches.
    scenarioPath = std::string(msg->argv[7]);
    std::tie(firstPersonIdx, firstLocationIdx, scenarioId) = buildCache(scenarioPath, numPeople, numPeoplePartitions, numLocations, numLocationPartitions, numDays);
  } else {
    scheduleType = (int) ScheduleType::syntheticSchedule;
    firstPersonIdx = 0;
    firstLocationIdx = 0;
  }

  // Detemine which contact modle to use
  contactModelType = (int) ContactModelType::constant_probability;
  if (msg->argc >= 9) {
    std::string tmp = std::string(msg->argv[8]);
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

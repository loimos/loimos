/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */
#include "absl/flags/flag.h"
#include "DiseaseModel.h"
#include "Locations.h"
#include "Main.h"
#include "People.h"
#include "contact_model/ContactModel.h"
#include "loimos.decl.h"
#include "readers/Preprocess.h"

#include <fstream>
#include <iostream>
#include <string>
#include <tuple>

#define LOIMOS_TESTING
#ifdef LOIMOS_TESTING
#include "gtest/gtest.h"
#endif

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
/* readonly */ uint64_t totalVisits;
/* readonly */ double simulationStartTime;
/* readonly */ double iterationStartTime;

// For synthetic run.
/* readonly */ int synPeopleGridWidth;
/* readonly */ int synPeopleGridHeight;
/* readonly */ int synLocationGridWidth;
/* readonly */ int synLocationGridHeight;
/* readonly */ int synLocalLocationGridWidth;
/* readonly */ int synLocalLocationGridHeight;
/* readonly */ int synLocationPartitionGridWidth;
/* readonly */ int synLocationPartitionGridHeight;
/* readonly */ int averageDegreeOfVisit;

Main::Main(CkArgMsg *msg) {
  // parsing command line arguments
  if (msg->argc < 7) {
    CkAbort(
        "Error, usage %s <people> <locations> <people subsets> <location "
        "subsets> <days> <disease_model_path> <scenario_folder (optional)>\n",
        msg->argv[0]);
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

    // Chare data
    synLocationPartitionGridWidth = atoi(msg->argv[7]);
    synLocationPartitionGridHeight = atoi(msg->argv[8]);
    numLocationPartitions =
        synLocationPartitionGridWidth * synLocationPartitionGridHeight;
    numPeoplePartitions = atoi(msg->argv[9]);

    // Calculate the dimensions of the block of locations stored by each
    // location chare
    synLocalLocationGridWidth = -1;
    if (0 == synLocationGridWidth % synLocationPartitionGridWidth) {
      synLocalLocationGridWidth =
          synLocationGridWidth / synLocationPartitionGridWidth;
    }
    synLocalLocationGridHeight = -1;
    if (0 == synLocationGridHeight % synLocationPartitionGridHeight) {
      synLocalLocationGridHeight =
          synLocationGridHeight / synLocationPartitionGridHeight;
    }

    if (-1 == synLocalLocationGridWidth || -1 == synLocalLocationGridHeight) {
      CkAbort("Error: dimensions of location chare grid must divide those of "
              "location grid:\r\nchare grid is %d by %d, location grid is %d "
              "by %d\r\n",
              synLocationPartitionGridWidth, synLocationPartitionGridHeight,
              synLocationGridWidth, synLocationGridHeight);
    }

    baseRunInfo = 9;
  } else {
    numPeople = atoi(msg->argv[2]);
    numLocations = atoi(msg->argv[3]);
    numPeoplePartitions = atoi(msg->argv[4]);
    numLocationPartitions = atoi(msg->argv[5]);
    baseRunInfo = 5;
  }

  numDays = atoi(msg->argv[baseRunInfo + 1]);
  pathToOutput = std::string(msg->argv[baseRunInfo + 2]);
  std::string pathToDiseaseModel = std::string(msg->argv[baseRunInfo + 3]);

  // Handle both real data runs or runs using synthetic populations.
  if (syntheticRun) {
    firstPersonIdx = 0;
    firstLocationIdx = 0;
  } else {
    // Create data caches.
    scenarioPath = std::string(msg->argv[baseRunInfo + 4]);
    std::tie(firstPersonIdx, firstLocationIdx, scenarioId) =
        buildCache(scenarioPath, numPeople, numPeoplePartitions, numLocations,
                   numLocationPartitions, numDays);
  }

  // Detemine which contact modle to use
  contactModelType = (int)ContactModelType::constant_probability;
  if (msg->argc == baseRunInfo + 6) {
    std::string tmp = std::string(msg->argv[baseRunInfo + 5]);
    // We can just use a flag for now in the CLI, since we only have two
    // models and that's easier to parse, but we may eventually have more,
    // which is why we use an enum to actually hold the model value
    if ("-m" == tmp or "--min-max-alpha" == tmp) {
      contactModelType = (int)ContactModelType::min_max_alpha;
    }
  }

  // setup main proxy
  CkPrintf("\nRunning Loimos on %d PEs with %d people, %d locations, %d people "
           "chares, %d location chares, and %d days\n",
           CkNumPes(), numPeople, numLocations, numPeoplePartitions,
           numLocationPartitions, numDays);
  mainProxy = thisProxy;

  if (syntheticRun) {
    CkPrintf("Synthetic run with (%d, %d) person grid and (%d, %d) location "
             "grid. Average degree of %d\n\n",
             synPeopleGridWidth, synPeopleGridHeight, synLocationGridWidth,
             synLocationGridHeight, averageDegreeOfVisit);
  }

  // Instantiate DiseaseModel nodegroup (One for each physical processor).
  CkPrintf("Loading diseaseModel at %s.\n", pathToDiseaseModel.c_str());
  globDiseaseModel = CProxy_DiseaseModel::ckNew(pathToDiseaseModel);
  diseaseModel = globDiseaseModel.ckLocalBranch();
  accumulated.resize(diseaseModel->getNumberOfStates(), 0);
  delete msg;

  // creating chare arrays
  if (!syntheticRun) {
    CkPrintf("Loading people and locations from %s.\n", scenarioPath.c_str());
  }

  peopleArray = CProxy_People::ckNew(numPeoplePartitions);
  locationsArray = CProxy_Locations::ckNew(numLocationPartitions);

// Run pre-run unit tests.
#ifdef LOIMOS_TESTING
  testing::InitGoogleTest(&msg->argc, msg->argv);
  if (RUN_ALL_TESTS() != 0) {
    CkAbort("Failed unit tests!\n");
  }
#endif

  // run
  CkPrintf("Running ...\n\n");
  simulationStartTime = CkWallTimer();
  mainProxy.run();
}

void Main::SaveStats(int *data) {
  DiseaseModel *diseaseModel = globDiseaseModel.ckLocalBranch();
  int numDiseaseStates = diseaseModel->getNumberOfStates();

  // Open output csv
  std::ofstream outFile(pathToOutput);
  if (!outFile) {
    CkAbort("Error: invalid output path, %s\n", pathToOutput.c_str());
  }

  // Write header row
  outFile << "day,state,total_in_state,change_in_state" << std::endl;

  for (day = 0; day < numDays; ++day, data += numDiseaseStates + 1) {
    // Get total visits for the day.
    totalVisits += data[0];

    // Get number of disease state changes.
    for (int i = 0; i < numDiseaseStates; i++) {
      int total_in_state = data[i + 1];
      int change_in_state = total_in_state - accumulated[i];
      if (total_in_state != 0 || change_in_state != 0) {
        // Write out data for state on that day
        outFile << day << "," << diseaseModel->lookupStateName(i) << ","
                << total_in_state << "," << change_in_state << std::endl;
      }
      accumulated[i] = total_in_state;
    }
  }

  outFile.close();
}

#include "loimos.def.h"

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

#include <string>
#include <tuple>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sstream>
#include <unordered_map>
#include <sys/time.h>
#include <sys/resource.h>

#ifdef USE_HYPERCOMM
#include "Aggregator.h"
#include <hypercomm/registration.hpp>
#endif

#ifdef ENABLE_UNIT_TESTING
#include "gtest/gtest.h"
#endif

/* readonly */ CProxy_Main mainProxy;
/* readonly */ CProxy_People peopleArray;
/* readonly */ CProxy_Locations locationsArray;
#ifdef USE_HYPERCOMM
/* readonly */ CProxy_Aggregator aggregatorProxy;
#endif
/* readonly */ CProxy_DiseaseModel globDiseaseModel;
/* readonly */ CProxy_TraceSwitcher traceArray;
/* readonly */ int numPeople;
/* readonly */ int numLocations;
/* readonly */ int numPeoplePartitions;
/* readonly */ int numLocationPartitions;
/* readonly */ int numDays;
/* readonly */ int numDaysWithRealData;
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
/* readonly */ bool interventionStategy;

class TraceSwitcher : public CBase_TraceSwitcher {
  public:
    #ifdef USE_PROJECTIONS
    TraceSwitcher() : CBase_TraceSwitcher(){}
    
    void traceOn(){
      traceBegin();
      contribute(CkCallback(CkReductionTarget(Main, traceSwitchOn),mainProxy));
    };
    
    void traceOff(){
      traceEnd();    
      contribute(CkCallback(CkReductionTarget(Main, traceSwitchOff),mainProxy));
    };   
    
    void traceFlush(){
      traceEnd();
      traceFlushLog();
      traceBegin();
    };
    
    void reportMemoryUsage(){ 
      // Find this process's memory usage
      struct rusage self_usage;
      getrusage(RUSAGE_SELF, &self_usage);
      
      // Account for the fact that multiple threads may be run on this process
      self_usage.ru_maxrss /= CkNodeSize(CkMyNode());

      pid_t pid = getpid();
      long result[2];
      result[0] = pid;
      result[1] = self_usage.ru_maxrss;
      
      CkCallback cb(CkReductionTarget(Main, printMemoryUsage), mainProxy);
      contribute(sizeof(long), &self_usage.ru_maxrss, CkReduction::sum_long, cb);

      //CkPrintf("  Process %ld is using %ld kb\n",
      //    (int) pid, self_usage.ru_maxrss);
    };
    #endif // USE_PROJECTIONS
   
    #ifdef ENABLE_LB
    void instrumentOn(){
      LBTurnInstrumentOn();
      contribute(CkCallback(CkReductionTarget(Main, instrumentSwitchOn),
            mainProxy));
    }
    
    void instrumentOff(){
      LBTurnInstrumentOff();
      contribute(CkCallback(CkReductionTarget(Main, instrumentSwitchOff),
            mainProxy));
    }
    #endif // ENABLE_LB
};

Main::Main(CkArgMsg* msg) {
  // parsing command line arguments
  if(msg->argc < 7){
    CkAbort("Error, usage %s <people> <locations> <people subsets> <location subsets> <days> <disease_model_path> <scenario_folder (optional)>\n", msg->argv[0]);
  }

  /*
  for (int i = 0; i < msg->argc; ++i) {
    CkPrintf("argv[%d]: %s\n", i, msg->argv[i]);
  }
  */
  
  int argNum = 0;
  syntheticRun = atoi(msg->argv[++argNum]) == 1;
  int baseRunInfo = 0;
  if (syntheticRun) {
    // Get number of people.
    synPeopleGridWidth = atoi(msg->argv[++argNum]);
    synPeopleGridHeight = atoi(msg->argv[++argNum]);
    numPeople = synPeopleGridWidth * synPeopleGridHeight;
    
    // Location data
    synLocationGridWidth = atoi(msg->argv[++argNum]);
    synLocationGridHeight = atoi(msg->argv[++argNum]);
    numLocations = synLocationGridWidth * synLocationGridHeight;
    assert(synPeopleGridWidth >= synLocationGridWidth);
    assert(synPeopleGridHeight >= synLocationGridHeight);

    // Edge degree.
    averageDegreeOfVisit = atoi(msg->argv[++argNum]);
    
    // Chare data
    synLocationPartitionGridWidth = atoi(msg->argv[++argNum]);
    synLocationPartitionGridHeight = atoi(msg->argv[++argNum]);
    numLocationPartitions =
      synLocationPartitionGridWidth * synLocationPartitionGridHeight;
    numPeoplePartitions = atoi(msg->argv[++argNum]);
 
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
      CkAbort("Error: dimensions of location chare grid must divide those of location grid:\r\nchare grid is %d by %d, location grid is %d by %d\r\n",
        synLocationPartitionGridWidth, synLocationPartitionGridHeight,
        synLocationGridWidth, synLocationGridHeight);
    }
    numDays = atoi(msg->argv[++argNum]);
    baseRunInfo = 10;
  } else {
    numPeople = atoi(msg->argv[++argNum]);
    numLocations = atoi(msg->argv[++argNum]);
    numPeoplePartitions = atoi(msg->argv[++argNum]);
    numLocationPartitions = atoi(msg->argv[++argNum]);
    numDays = atoi(msg->argv[++argNum]);
    numDaysWithRealData = atoi(msg->argv[++argNum]);
    baseRunInfo = 7;
  }
  
  pathToOutput = std::string(msg->argv[++argNum]);
  std::string pathToDiseaseModel = std::string(msg->argv[++argNum]);
  CkPrintf("Disease model is at %s\n", msg->argv[argNum]);

  // Handle both real data runs or runs using synthetic populations.
  if(syntheticRun) {
    firstPersonIdx = 0;
    firstLocationIdx = 0;
  } else {    
    // Create data caches.
    scenarioPath = std::string(msg->argv[++argNum]);
    std::tie(firstPersonIdx, firstLocationIdx, scenarioId) = buildCache(
        scenarioPath, numPeople, numPeoplePartitions, numLocations,
        numLocationPartitions, numDays);
  }

  // Detemine which contact modle to use
  contactModelType = (int) ContactModelType::constant_probability;
  interventionStategy = false;
  int interventionStategyLocation = -1;
  for (; argNum < msg->argc; ++argNum) {
    std::string tmp = std::string(msg->argv[argNum]);
    if ("-m" == tmp or "--min-max-alpha" == tmp) {
      contactModelType = (int) ContactModelType::min_max_alpha;
    } else if ("-i" == tmp && argNum + 1 < msg->argc) {
      interventionStategyLocation = ++argNum;
      interventionStategy = true;
    }
  }

  // setup main proxy
  CkPrintf("\nRunning Loimos on %d PEs with %d people, %d locations, %d people chares, %d location chares, and %d days\n", CkNumPes(), numPeople, numLocations, numPeoplePartitions, numLocationPartitions, numDays);
  mainProxy = thisProxy;

  if(syntheticRun) {
    CkPrintf("Synthetic run with (%d, %d) person grid and (%d, %d) location grid. Average degree of %d\n\n", synPeopleGridWidth, synPeopleGridHeight, synLocationGridWidth, synLocationGridHeight, averageDegreeOfVisit);
  }

#ifdef ENABLE_UNIT_TESTING
  CkPrintf("Executing unit testing.");
  testing::InitGoogleTest(&msg->argc, msg->argv);
  RUN_ALL_TESTS();
#endif

  // Instantiate DiseaseModel nodegroup (One for each physical processor).
  CkPrintf("Loading diseaseModel at %s.\n", pathToDiseaseModel.c_str());
  if (interventionStategy) {
    CkPrintf("intervention stategy index: %d\n", interventionStategyLocation);
    globDiseaseModel = CProxy_DiseaseModel::ckNew(pathToDiseaseModel,
        scenarioPath, msg->argv[interventionStategyLocation]);
    CkPrintf("Loading intervention at %s.\n",
        msg->argv[interventionStategyLocation]);

  } else {
    globDiseaseModel = CProxy_DiseaseModel::ckNew(pathToDiseaseModel,
        scenarioPath, "");
    CkPrintf("Running with no intervention.\n");
  }
  
  diseaseModel = globDiseaseModel.ckLocalBranch();
  accumulated.resize(diseaseModel->getNumberOfStates(), 0);
  delete msg;

  // creating chare arrays
  if (!syntheticRun) {
    CkPrintf("Loading people and locations from %s.\n", scenarioPath.c_str());
  }

  chareCount = 3; // Number of chare arrays/groups
  createdCount = 0;

  peopleArray = CProxy_People::ckNew(numPeoplePartitions);
  locationsArray = CProxy_Locations::ckNew(numLocationPartitions);

#ifdef USE_HYPERCOMM
  // Create Hypercomm message aggregators using env variables
  AggregatorParam visitParams;
  AggregatorParam interactParams;
  char* env_p;
  if (env_p = std::getenv("HC_VISIT_PARAMS")) {
    std::string env_str(env_p);
    std::vector<std::string> tokens;
    std::stringstream env_ss(env_str);
    std::string token;

    while (getline(env_ss, token, ',')) {
      tokens.push_back(token);
    }

    CkAssert(tokens.size() == 5);
    int idx = 0;
    bool useAggregator = static_cast<bool>(std::stoi(tokens[idx++]));
    size_t bufferSize = static_cast<size_t>(std::stoi(tokens[idx++]));
    double threshold = std::stod(tokens[idx++]);
    double flushPeriod = std::stod(tokens[idx++]);
    bool nodeLevel = static_cast<bool>(std::stoi(tokens[idx++]));
    visitParams = AggregatorParam(useAggregator, bufferSize, threshold,
        flushPeriod, nodeLevel);
  }
  if (env_p = std::getenv("HC_INTERACT_PARAMS")) {
    std::string env_str(env_p);
    std::vector<std::string> tokens;
    std::stringstream env_ss(env_str);
    std::string token;

    while (getline(env_ss, token, ',')) {
      tokens.push_back(token);
    }

    CkAssert(tokens.size() == 5);
    int idx = 0;
    bool useAggregator = static_cast<bool>(std::stoi(tokens[idx++]));
    size_t bufferSize = static_cast<size_t>(std::stoi(tokens[idx++]));
    double threshold = std::stod(tokens[idx++]);
    double flushPeriod = std::stod(tokens[idx++]);
    bool nodeLevel = static_cast<bool>(std::stoi(tokens[idx++]));
    interactParams = AggregatorParam(useAggregator, bufferSize, threshold,
        flushPeriod, nodeLevel);
  }

  aggregatorProxy = CProxy_Aggregator::ckNew(visitParams, interactParams);
}

void Main::CharesCreated() {
  if (++createdCount == chareCount) {
#endif //USE_HYPERCOMM
    // Run
    CkPrintf("Running ...\n\n");
    simulationStartTime = CkWallTimer();
    mainProxy.run();
#ifdef USE_HYPERCOMM
  }
#endif
}

void Main::SaveStats(int *data) {
  DiseaseModel* diseaseModel = globDiseaseModel.ckLocalBranch();
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
        outFile << day << ","
          << diseaseModel->lookupStateName(i) << ","
          << total_in_state << ","
          << change_in_state << std::endl;
      }
      accumulated[i] = total_in_state;
    }
  }

  outFile.close();
}

#include "loimos.def.h"

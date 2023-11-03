/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "Main.h"
#include "Types.h"
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
#include <limits>
#include <vector>
#include <random>
#include <memory>
#include <unordered_set>
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
/* readonly */ Id numPeople;
/* readonly */ Id numLocations;
/* readonly */ PartitionId numPersonPartitions;
/* readonly */ PartitionId numLocationPartitions;
/* readonly */ int numDays;
/* readonly */ int numDaysWithDistinctVisits;
/* readonly */ bool syntheticRun;
/* readonly */ int contactModelType;
/* readonly */ int maxSimVisitsIdx;
/* readonly */ int ageIdx;
/* readonly */ Counter totalVisits;
/* readonly */ Counter totalInteractions;
/* readonly */ Counter totalExposures;
/* readonly */ double simulationStartTime;
/* readonly */ double iterationStartTime;
/* readonly */ double stepStartTime;
/* readonly */ double dataLoadingStartTime;
/* readonly */ std::vector<double> totalTime;


// For synthetic run.
/* readonly */ Id synPeopleGridWidth;
/* readonly */ Id synPeopleGridHeight;
/* readonly */ Id synLocationGridWidth;
/* readonly */ Id synLocationGridHeight;
/* readonly */ Id synLocalLocationGridWidth;
/* readonly */ Id synLocalLocationGridHeight;
/* readonly */ PartitionId synLocationPartitionGridWidth;
/* readonly */ PartitionId synLocationPartitionGridHeight;
/* readonly */ int averageDegreeOfVisit;
/* readonly */ bool interventionStategy;

class TraceSwitcher : public CBase_TraceSwitcher {
 public:
  #ifdef ENABLE_TRACING
  TraceSwitcher() : CBase_TraceSwitcher() {}
  void traceOn() {
    traceBegin();
    contribute(CkCallback(CkReductionTarget(Main, traceSwitchOn), mainProxy));
  }
  void traceOff() {
    traceEnd();
    contribute(CkCallback(CkReductionTarget(Main, traceSwitchOff), mainProxy));
  }
  void traceFlush() {
    traceEnd();
    traceFlushLog();
    traceBegin();
  }
  #if ENABLE_TRACING == TRACE_MEMORY
  void reportMemoryUsage() {
    // Find this process's memory usage
    struct rusage self_usage;
    getrusage(RUSAGE_SELF, &self_usage);
    // Account for the fact that multiple threads may be run on this process
    self_usage.ru_maxrss /= CkNodeSize(CkMyNode());
    pid_t pid = getpid();
    int32_t result[2];
    result[0] = pid;
    result[1] = self_usage.ru_maxrss;
    CkCallback cb(CkReductionTarget(Main, printMemoryUsage), mainProxy);
    contribute(sizeof(int32_t), &self_usage.ru_maxrss, CkReduction::sum_long, cb);
    #if ENABLE_DEBUG >= DEBUG_BY_CHARE
      CkPrintf("  Process %ld is using %ld kb\n",
          static_cast<int>(pid), self_usage.ru_maxrss);
    #endif
  }
  #endif  // ENABLE_TRACING == TRACE_MEMORY
  #endif  // ENABLE_TRACING
  #ifdef ENABLE_LB
  void instrumentOn() {
    LBTurnInstrumentOn();
    contribute(CkCallback(CkReductionTarget(Main, instrumentSwitchOn),
          mainProxy));
  }
  void instrumentOff() {
    LBTurnInstrumentOff();
    contribute(CkCallback(CkReductionTarget(Main, instrumentSwitchOff),
          mainProxy));
  }
  #endif  // ENABLE_LB
};

Main::Main(CkArgMsg* msg) {
  // parsing command line arguments
  if (msg->argc < 7) {
    CkAbort("Error, usage %s <people> <locations> <people subsets> <location subsets>"
    " <days> <disease_model_path> <scenario_folder (optional)>\n", msg->argv[0]);
  }
#ifdef ENABLE_DEBUG
  CkPrintf("Debug printing enabled (verbosity at level %d)\n", ENABLE_DEBUG);
#endif
#if ENABLE_DEBUG >= DEBUG_VERBOSE
  for (int i = 0; i < msg->argc; ++i) {
    CkPrintf("argv[%d]: %s\n", i, msg->argv[i]);
  }
#endif

  dataLoadingStartTime = CkWallTimer();

  int argNum = 0;
  syntheticRun = atoi(msg->argv[++argNum]) == 1;

  if (syntheticRun) {
    // Get number of people.
    synPeopleGridWidth = atol(msg->argv[++argNum]);
    synPeopleGridHeight = atol(msg->argv[++argNum]);
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
    numPersonPartitions = atoi(msg->argv[++argNum]);

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
      CkAbort("Error: dimensions of location chare grid must divide those "
          "of location grid:\r\nchare grid is %d by %d, location grid is %d by %d\r\n",
        synLocationPartitionGridWidth, synLocationPartitionGridHeight,
        synLocationGridWidth, synLocationGridHeight);
    }

    numDays = atoi(msg->argv[++argNum]);
    numDaysWithDistinctVisits = 7;

  } else {
    numPeople = atoi(msg->argv[++argNum]);
    numLocations = atoi(msg->argv[++argNum]);
    numPersonPartitions = atoi(msg->argv[++argNum]);
    numLocationPartitions = atoi(msg->argv[++argNum]);
    numDays = atoi(msg->argv[++argNum]);
    numDaysWithDistinctVisits = atoi(msg->argv[++argNum]);
  }

  if (numPeople < numPersonPartitions) {
    CkAbort("Error: running on more people chares (%d) than people (%d)",
        numPersonPartitions, numPeople);
  }
  if (numLocations < numLocationPartitions) {
    CkAbort("Error: running on more location chares (%d) than locations (%d)",
        numLocationPartitions, numLocations);
  }

  pathToOutput = std::string(msg->argv[++argNum]);
#if ENABLE_DEBUG >= DEBUG_BASIC
  CkPrintf("Saving simulation output to %s\n", msg->argv[argNum]);
#endif
  std::string pathToDiseaseModel = std::string(msg->argv[++argNum]);
#if ENABLE_DEBUG >= DEBUG_BASIC
  CkPrintf("Reading disease model from %s\n", msg->argv[argNum]);
#endif

  // Handle both real data runs or runs using synthetic populations.
  std::string scenarioPath;
  std::string scenarioId;
  if (!syntheticRun) {
    // Create data caches.
    scenarioPath = std::string(msg->argv[++argNum]);

    // This allows users to omit the trailing "/" from the scenario path
    // while still allowing us to find the files properly
    if (scenarioPath.back() != '/') {
      scenarioPath.push_back('/');
    }
  }

  // Detemine which contact modle to use
  contactModelType = static_cast<int>(ContactModelType::constant_probability);
  interventionStategy = false;
  int interventionStategyLocation = -1;
  for (; argNum < msg->argc; ++argNum) {
    std::string tmp = std::string(msg->argv[argNum]);

    // We can just use a flag for now in the CLI, since we only have two
    // models and that's easier to parse, but we may eventually have more,
    // which is why we use an enum to actually hold the model value
    if ("-m" == tmp || "--min-max-alpha" == tmp) {
      contactModelType = static_cast<int>(ContactModelType::min_max_alpha);

    } else if ("-i" == tmp && argNum + 1 < msg->argc) {
      interventionStategyLocation = ++argNum;
      interventionStategy = true;
    }
  }

  // setup main proxy
  CkPrintf("\nRunning Loimos on %d PEs with %d people, %d locations, "
      "%d people chares, %d location chares, and %d days\n",
    CkNumPes(), numPeople, numLocations, numPersonPartitions,
    numLocationPartitions, numDays);
  mainProxy = thisProxy;

  if (syntheticRun) {
    CkPrintf("Synthetic run with (%d, %d) person grid and "
        "(%d, %d) location grid. Average degree of %d\n\n",
      synPeopleGridWidth, synPeopleGridHeight, synLocationGridWidth,
      synLocationGridHeight, averageDegreeOfVisit);
  }

#ifdef ENABLE_UNIT_TESTING
  CkPrintf("Executing unit testing.");
  testing::InitGoogleTest(&msg->argc, msg->argv);
  RUN_ALL_TESTS();
#endif

  // Instantiate DiseaseModel nodegroup (One for each physical processor).
#if ENABLE_DEBUG >= DEBUG_BASIC
  CkPrintf("Loading diseaseModel at %s.\n", pathToDiseaseModel.c_str());
#endif
  if (interventionStategy) {
#if ENABLE_DEBUG >= DEBUG_BASIC
    CkPrintf("intervention stategy index: %d\n", interventionStategyLocation);
#endif
    globDiseaseModel = CProxy_DiseaseModel::ckNew(pathToDiseaseModel,
        scenarioPath, msg->argv[interventionStategyLocation]);
#if ENABLE_DEBUG >= DEBUG_BASIC
    CkPrintf("Loading intervention at %s.\n",
        msg->argv[interventionStategyLocation]);
#endif

  } else {
    globDiseaseModel = CProxy_DiseaseModel::ckNew(pathToDiseaseModel,
        scenarioPath, "");
    CkPrintf("Running with no intervention.\n");
  }

  diseaseModel = globDiseaseModel.ckLocalBranch();
  accumulated.resize(diseaseModel->getNumberOfStates(), 0);
  delete msg;

#if ENABLE_DEBUG >= DEBUG_BASIC
  CkPrintf("Person Attributes:\n");
  for (int i = 0; i < diseaseModel->personAttributes.size(); i++) {
    CkPrintf("(%d) %s: default: %lf, type: %d\n",
        i, diseaseModel->personAttributes.getName(i).c_str(),
        diseaseModel->personAttributes.getDefaultValueAsDouble(i),
        diseaseModel->personAttributes.getDataType(i));
  }

  CkPrintf("Locations Attributes:\n");
  for (int i = 0; i < diseaseModel->locationAttributes.size(); i++) {
    CkPrintf("(%d) %s: default: %lf, type: %d\n",
        i, diseaseModel->locationAttributes.getName(i).c_str(),
        diseaseModel->locationAttributes.getDefaultValueAsDouble(i),
        diseaseModel->locationAttributes.getDataType(i));
  }
#endif

  CkPrintf("\nFinished loading shared/global data in %lf seconds.\n",
      CkWallTimer() - dataLoadingStartTime);

  // creating chare arrays
  if (!syntheticRun) {
#if ENABLE_DEBUG >= DEBUG_BASIC
    CkPrintf("Loading people and locations from %s.\n", scenarioPath.c_str());
#endif
  }

  chareCount = numPersonPartitions;  // Number of chare arrays/groups
  createdCount = 0;

  dataLoadingStartTime = CkWallTimer();

#ifdef ENABLE_RANDOM_SEED
  seed = time(NULL);
#else
  seed = 0;
#endif

  peopleArray = CProxy_People::ckNew(seed, scenarioPath, numPersonPartitions);
  locationsArray = CProxy_Locations::ckNew(seed, scenarioPath,
      numLocationPartitions);

#ifdef ENABLE_TRACING
  traceArray = CProxy_TraceSwitcher::ckNew();
#endif

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
#endif  // USE_HYPERCOMM
}

void Main::CharesCreated() {
  // CkPrintf("  %d of %d chares created\n", createdCount, chareCount);
  if (++createdCount == chareCount) {
    CkPrintf("\nFinished loading people and location data in %lf seconds.\n",
        CkWallTimer() - dataLoadingStartTime);

    mainProxy.run();
  }
}

void Main::SeedInfections() {
  std::default_random_engine generator(seed);

  // Determine all of the intitial infections on the first day so we can
  // guarentee they are unique (not checking this quickly runs into birthday
  // problem issues, even for sizable datasets)
  if (0 == day) {
    Id firstPersonIdx = diseaseModel->getGlobalLocationIndex(0, 0);
    std::uniform_int_distribution<Id> personDistrib(firstPersonIdx,
        firstPersonIdx + numPeople - 1);
    std::unordered_set<Id> initialInfectionsSet;
    initialInfections.reserve(INITIAL_INFECTIONS);
    // Use set to check membership becuase it's faster and we can spare the
    // memory; INITIAL_INFECTIONS should be fairly small
    initialInfectionsSet.reserve(INITIAL_INFECTIONS);
    while (initialInfectionsSet.size() < INITIAL_INFECTIONS
        // This loop will go forever on small test populations without
        // this check
        && initialInfectionsSet.size() < numPeople) {
      Id personIdx = personDistrib(generator);
      if (initialInfectionsSet.count(personIdx) == 0) {
        initialInfections.emplace_back(personIdx);
        initialInfectionsSet.emplace(personIdx);
      }
    }
  }

  // Check for empty is to avoid issues with small test populations
  for (int i = 0; i < INITIAL_INFECTIONS_PER_DAY && !initialInfections.empty();
      ++i) {
    Id personIdx = initialInfections.back();
    initialInfections.pop_back();

    PartitionId peoplePartitionIdx = diseaseModel->getPersonPartitionIndex(personIdx);

    // Make a super contagious visit for that person.
    std::vector<Interaction> interactions;
    interactions.emplace_back(
      std::numeric_limits<double>::max(), 0, 0, 0, std::numeric_limits<int>::max());

    InteractionMessage interMsg(-1, personIdx, interactions);
    #ifdef USE_HYPERCOMM
    Aggregator* agg = aggregatorProxy.ckLocalBranch();
    if (agg->interact_aggregator) {
      agg->interact_aggregator->send(peopleArray[peoplePartitionIdx], interMsg);
    } else {
    #endif  // USE_HYPERCOMM
      peopleArray[peoplePartitionIdx].ReceiveInteractions(interMsg);
    #ifdef USE_HYPERCOMM
    }
    #endif  // USE_HYPERCOMM
  }
}

void Main::SaveStats(Id *data) {
  DiseaseModel* diseaseModel = globDiseaseModel.ckLocalBranch();
  DiseaseState numDiseaseStates = diseaseModel->getNumberOfStates();

  // Open output csv
  std::ofstream outFile(pathToOutput);
  if (!outFile) {
    CkAbort("Error: invalid output path, %s\n", pathToOutput.c_str());
  }

  // Write header row
  outFile << "day,state,total_in_state,change_in_state" << std::endl;

  for (day = 0; day < numDays; ++day, data += numDiseaseStates) {
    // Get number of disease state changes.
    for (DiseaseState i = 0; i < numDiseaseStates; i++) {
      Id num_in_state = data[i];
      Id change_in_state = num_in_state - accumulated[i];
      if (num_in_state != 0 || change_in_state != 0) {
        // Write out data for state on that day
        outFile << day << ","
          << diseaseModel->lookupStateName(i) << ","
          << num_in_state << ","
          << change_in_state << std::endl;
      }
      accumulated[i] = num_in_state;
    }
  }

  outFile.close();
  // CkPrintf("  Found %lu visits and %lu interactions in summaries\n",
  //     numVisits, numInteractions);
}

#include "loimos.def.h"

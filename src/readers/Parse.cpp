/* Copyright 2020-2024 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "../Types.h"
#include "../contact_model/ContactModel.h"
#include "charm++.h"

#include <vector>
#include <string>
#include <cstdlib>
#include <sys/time.h>

void parse(int argc, char **argv, Arguments *args) {
  if (argc < 7) {
    CkAbort("Error, usage %s <people> <locations> <people subsets> <location subsets>"
    " <days> <disease_model_path> <scenario_folder (optional)>\n", argv[0]);
  }
#if ENABLE_DEBUG >= DEBUG_VERBOSE
  for (int i = 0; i < argc; ++i) {
    CkPrintf("argv[%d]: %s\n", i, argv[i]);
  }
#endif
  int argNum = 0;
  args->isOnTheFlyRun = atoi(argv[++argNum]) == 1;

  if (args->isOnTheFlyRun) {
    OnTheFlyArguments *onTheFly = &args->onTheFly;

    // Get number of people.
    onTheFly->personGrid.width = atol(argv[++argNum]);
    onTheFly->personGrid.height = atol(argv[++argNum]);

    // Location data
    onTheFly->locationGrid.width = atoi(argv[++argNum]);
    onTheFly->locationGrid.height = atoi(argv[++argNum]);

    if (!(onTheFly->personGrid >= onTheFly->locationGrid)) {
      CkAbort("Error: dimensions of people grid ("
        ID_PRINT_TYPE " by " ID_PRINT_TYPE ")\n exceed those of location grid ("
        ID_PRINT_TYPE " by " ID_PRINT_TYPE ") in at least one dimension\n",
        onTheFly->personGrid.width, onTheFly->personGrid.height,
        onTheFly->locationGrid.width, onTheFly->locationGrid.height
      );
    }

    // Edge degree.
    onTheFly->averageVisitsPerDay = atoi(argv[++argNum]);

    // Chare data
    onTheFly->locationPartitionGrid.width = atoi(argv[++argNum]);
    onTheFly->locationPartitionGrid.height = atoi(argv[++argNum]);
    args->numLocationPartitions = onTheFly->locationPartitionGrid.area();
    args->numPersonPartitions = atoi(argv[++argNum]);

    // For simplicity, the current scheme assumes each local grid
    // is the same size
    if (onTheFly->locationGrid.isDivisible(onTheFly->locationPartitionGrid)) {
      onTheFly->localLocationGrid.width = onTheFly->locationGrid.width
        / onTheFly->locationPartitionGrid.width;
      onTheFly->localLocationGrid.height = onTheFly->locationGrid.height
        / onTheFly->localLocationGrid.height;

    } else {
      CkAbort("Error: dimensions of location chare grid must divide those "
          "of location grid:\nchare grid is "
          PARTITION_ID_PRINT_TYPE " by " PARTITION_ID_PRINT_TYPE
          ", location grid is " ID_PRINT_TYPE " by " ID_PRINT_TYPE "\n",
        onTheFly->locationPartitionGrid.width, onTheFly->locationPartitionGrid.height,
        onTheFly->locationGrid.width, onTheFly->locationGrid.height);
    }

    args->numDays = atoi(argv[++argNum]);
    args->numDaysWithDistinctVisits = 7;
    args->scenarioPath = std::string("");

  } else {
    args->numPersonPartitions = atoi(argv[++argNum]);
    args->numLocationPartitions = atoi(argv[++argNum]);
    args->numDays = atoi(argv[++argNum]);
    args->numDaysWithDistinctVisits = atoi(argv[++argNum]);
  }

  args->outputPath = std::string(argv[++argNum]);
  args->diseasePath = std::string(argv[++argNum]);

  // Handle both real data runs or runs using synthetic populations.
  if (!args->isOnTheFlyRun) {
    // Create data caches.
    args->scenarioPath = std::string(argv[++argNum]);

    // This allows users to omit the trailing "/" from the scenario path
    // while still allowing us to find the files properly
    if (args->scenarioPath.back() != '/') {
      args->scenarioPath.push_back('/');
    }
  }
#ifdef OUTPUT_FLAGS
  if (args->outputPath.back() == '/') {
    args->outputPath.pop_back();
  }
  create_directory(args->outputPath, args->isOnTheFlyRun ? "." : args->scenarioPath);
  args->outputPath.push_back('/');
#endif

  // Optional arguments
  args->contactModelType = static_cast<int>(ContactModelType::constant_probability);
  args->hasIntervention = false;
  for (; argNum < argc; ++argNum) {
    std::string tmp = std::string(argv[argNum]);

    // We can just use a flag for now in the CLI, since we only have two
    // models and that's easier to parse, but we may eventually have more,
    // which is why we use an enum to actually hold the model value
    if ("-m" == tmp || "--min-max-alpha" == tmp) {
      args->contactModelType = static_cast<int>(ContactModelType::min_max_alpha);

    } else if ("-i" == tmp && argNum + 1 < argc) {
      args->interventionPath = std::string(argv[++argNum]);
      args->hasIntervention = true;
    }
  }

#if ENABLE_DEBUG
  CkPrintf("Saving simulation output to %s\n", args->outputPath);
  CkPrintf("Reading disease model from %s\n", args->diseasePath);
  if (args->hasIntervention) {
    CkPrintf("Reading intervention model from %s\n", args->interventionPath);
  }
  if (!args->isOnTheFlyRun) {
    CkPrintf("Loading people and locations from %s.\n", args->scenarioPath.c_str());
  }
#endif
  
#ifdef ENABLE_RANDOM_SEED
  args->seed = time(NULL);
#else
  args->seed = 0;
#endif

  args->numDaysToSeedOutbreak = 10;
#if OUTPUT_FLAGS & OUTPUT_OVERLAPS
  args->numInitialInfectionsPerDay = 0;
#else
  args->numInitialInfectionsPerDay = 2;
#endif
}
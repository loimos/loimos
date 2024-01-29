/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * This file is responsible for building a file cache pre-simulation to allow
 * direct fseeking in files rather than slow iteration. Three caches are built
 * for each of the people, locations, and activities file. The cache is
 * dependent on the files given and the number of chares
 */

#include "../loimos.decl.h"
#include "Preprocess.h"
#include "DataReader.h"
#include "../Defs.h"
#include "../Extern.h"
#include "../Location.h"
#include "../Person.h"
#include "../protobuf/data.pb.h"
#include "charm++.h"

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <tuple>
#include <sstream>
#include <sys/stat.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#define MAX_WRITE_SIZE 65536  // 2^16

/**
 * This file preprocesses a given input file.
 */
// TODO(IanCostello): Replace getline with function that doesn't need to copy to
//                    string object in subfunctions.
std::string buildCache(std::string scenarioPath,
    Id numPeople, const std::vector<Id> &personPartitionOffsets,
    Id numLocations, const std::vector<Id> &locationPartitionOffsets, int numDays) {
  PartitionId numPeopleChares = personPartitionOffsets.size();
  PartitionId numLocationChares = locationPartitionOffsets.size();

  // We need to uniquely identify this run configuration
  std::string uniqueScenario = getScenarioId(numPeople, numPeopleChares,
      numLocations, numLocationChares);
  CkPrintf("  Running with scenario id: %s\n", uniqueScenario.c_str());

  // Build person and location cache.
  buildObjectLookupCache(numPeople, personPartitionOffsets,
    scenarioPath + "people.textproto", scenarioPath + "people.csv",
    scenarioPath + uniqueScenario + "_people.cache");
  buildObjectLookupCache(numLocations,
    locationPartitionOffsets, scenarioPath + "locations.textproto",
    scenarioPath + "locations.csv", scenarioPath + uniqueScenario + "_locations.cache");
  buildActivityCache(numPeople, numDays, personPartitionOffsets[0],
    scenarioPath + "visits.textproto", scenarioPath + "visits.csv",
    scenarioPath + uniqueScenario + "_visits.cache");
  return uniqueScenario;
}

void buildObjectLookupCache(Id numObjs, const std::vector<Id> &offsets,
  std::string metadataPath, std::string inputPath, std::string outputPath) {
  /**
   * Assumptions: (about person file)
   * -- Contigious block of IDs that are sorted.
   *
   * Creates a mapping from char number to byte offset that the reading should
   * commence from.
   *
   * Returns:
   *    int offset of lowest contigious block.
   */
  // Open activity stream..
  std::ifstream activityStream(inputPath, std::ios_base::binary);

  // Read config file.
  loimos::proto::CSVDefinition csvDefinition;
  readProtobuf(metadataPath, &csvDefinition);

  int csvLocationOfPid = -1;
  for (int i = 0; i < csvDefinition.fields_size(); i += 1) {
    if (csvDefinition.fields(i).has_unique_id()) {
      csvLocationOfPid = i;
      break;
    }
  }
  assert(csvLocationOfPid != -1);

  std::string line;
  // Clear header.
  std::getline(activityStream, line);
  CacheOffset currentPosition = activityStream.tellg();

  // Check if file cache already created.
  std::ifstream existenceCheck(outputPath, std::ios_base::binary);
  if (existenceCheck.good()) {
    if (0 == CkMyNode()) {
      CkPrintf("  Using existing object cache\n");
    }
    existenceCheck.close();

  } else {
    CkPrintf("  Saving object cache to %s\n", outputPath.c_str());
    existenceCheck.close();
    std::ofstream outputStream(outputPath, std::ios_base::binary);
    for (PartitionId p = 0; p < offsets.size(); p++) {
      // Write current offset.
      outputStream.write(reinterpret_cast<char *>(&currentPosition),
        sizeof(CacheOffset));

      // Skip next n lines.
      // We already read the first location on the first chare to get
      // its id, so don't double count that line
      Id numObjs = getPartitionSize(p, numObjs, offsets);
      // - (0 == p);
      for (int i = 0; i < numObjs; i++) {
        std::getline(activityStream, line);
      }
      currentPosition = activityStream.tellg();
    }
    outputStream.flush();
  }
}

void buildActivityCache(Id numPeople, int numDays, Id firstPersonIdx,
  std::string metadataPath, std::string inputPath, std::string outputPath) {
  /**
   * Assumptions.
   * Stream is sorted by start time per person.
   */
  // Check if cache already created.
  std::ifstream existenceCheck(outputPath, std::ios_base::binary);
  if (existenceCheck.good()) {
    CkPrintf("  Using existing activity cache\n");
    return;
  }
  CkPrintf("  Saving activity cache to %s\n", outputPath.c_str());

  std::ifstream activityStream(inputPath, std::ios_base::binary);
  if (!activityStream) {
    CkAbort("Error: Could not open visit data input.\n");
  }

  // Read config file.
  loimos::proto::CSVDefinition csvDefinition;
  readProtobuf(metadataPath, &csvDefinition);

  // Create position vector for each person.
  std::size_t totalDataSize = numPeople * numDaysWithDistinctVisits
    * sizeof(CacheOffset);
  CacheOffset *elements = reinterpret_cast<CacheOffset *>(malloc(totalDataSize));
  if (NULL == elements) {
    CkAbort("Failed to malloc enoough memory for preprocessing.\n");
  }
  memset(elements, 0xFF, totalDataSize);

  // Various initialization.
  std::string line;
  // Clear header.
  std::getline(activityStream, line);
  CacheOffset current_position = activityStream.tellg();
  Id lastPerson = -1;
  Time lastTime = -1;
  Id nextPerson = 0;
  Time nextTime = 0;
  Time nextTimeSec = 0;
  Id locationId = -1;
  Time duration = -1;
  Id totalVisits = 0;
  // For better looping efficiency simulate one break of inner loop to start.
  std::tie(nextPerson, locationId, nextTime, duration) =
    parseActivityStream(&activityStream, &csvDefinition, NULL);
  nextTimeSec = nextTime;
  nextTime = getDay(nextTime);

  // Loop over the entire activity file and note boundaries on people and days
  Id numVisits = 0;
  while (!activityStream.eof()) {
    // CkPrintf("Person %d has %d visits on day %d (next byte is %u)\n",
    //   lastPerson, numVisits, lastTime, current_position);

    // Get details of new entry.
    lastPerson = nextPerson;
    lastTime = nextTime;
    numVisits = 0;

    CacheOffset index =
      numDaysWithDistinctVisits * (lastPerson - firstPersonIdx) + lastTime;
    if (numPeople * numDaysWithDistinctVisits > index) {
      elements[index] = current_position;
    } else {
      CkAbort("    Failed to write %lu bytes at %lu\n", current_position, index);
    }
    // if (0 == lastPerson % 10000) {
    //   CkPrintf("  Setting person %d to read from %u on day %d\n",
    //       lastPerson, current_position, lastTime);
    //   CkPrintf("  Person %d on day %d first visit (%u): %d to %d, at loc %d\n",
    //       lastPerson, lastTime, current_position, nextTimeSec,
    //       nextTimeSec + duration, locationId);
    // }

    // Scan until the next boundary.
    while (!activityStream.eof()
        && lastTime == nextTime
        && lastPerson == nextPerson) {
      current_position = activityStream.tellg();
      std::tie(nextPerson, locationId, nextTime, duration) =
        parseActivityStream(&activityStream,
            &csvDefinition, NULL);

      nextTime = getDay(nextTime);
      numVisits++;
      totalVisits++;
    }
  }
  CkPrintf("Parsed a total of " ID_PRINT_TYPE " visits\n", totalVisits);

  // Output
  std::ofstream outputStream(outputPath, std::ios::out | std::ios::binary);
  outputStream.write(reinterpret_cast<const char *>(elements), totalDataSize);
  outputStream.close();
}

int getDay(Time timeInSeconds) {
  return timeInSeconds / DAY_LENGTH;
}

std::string getScenarioId(Id numPeople, PartitionId numPeopleChares, Id numLocations,
    PartitionId numLocationChares) {
  std::ostringstream oss;
  oss << numPeople << "-" << numPeopleChares << "_" << numLocations
    << "-" << numLocationChares;
  return oss.str();
}

Id getFirstIndex(const loimos::proto::CSVDefinition *metadata, std::string inputPath) {
  std::ifstream activityStream(inputPath, std::ios_base::binary);

  // Skip header
  std::string line;
  std::getline(activityStream, line);

  // Find the id column
  std::getline(activityStream, line);
  char *str = strdup(line.c_str());
  char *tmp;
  char *tok = strtok_r(str, ",", &tmp);
  int i;
  for (i = 0; i < metadata->fields_size()
      && !metadata->fields(i).has_unique_id(); i++) {
    tok = strtok_r(tmp, ",", &tmp);
  }

  Id firstIdx;
  if (metadata->fields(i).has_unique_id()) {
    firstIdx = ID_PARSE(tok);
    free(str);
  } else {
    CkAbort("Error: no column in %s marked as id\n", inputPath.c_str());
  }

  return firstIdx;
}

bool create_directory(std::string path, std::string referencePath) {
  struct stat checkStat;
  struct stat referenceStat;

  int result = stat(path.c_str(), &checkStat);
  if (0 == result && S_ISDIR(checkStat.st_mode)) {
    CkPrintf("Directory %s already exists\n", path.c_str());
    return false;
  } else if (0 == result) {
    CkError("Attempted to create directory %s which exisits as a regular file\n",
        path.c_str());
  } else {
    stat(referencePath.c_str(), &referenceStat);
    mkdir(path.c_str(), referenceStat.st_mode);
    CkPrintf("Created directory %s\n", path.c_str());
    return true;
  }
}

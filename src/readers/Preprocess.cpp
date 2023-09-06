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
std::tuple<Id, Id, std::string> buildCache(std::string scenarioPath, Id numPeople,
    int numPeopleChares, Id numLocations, int numLocationChares, int numDays) {
  // We need to uniquely identify this run configuration
  std::string uniqueScenario = getScenarioId(numPeople, numPeopleChares,
      numLocations, numLocationChares);

  // Build person and location cache.
  Id firstPersonIdx = buildObjectLookupCache(numPeople, numPeopleChares,
    scenarioPath + "people.textproto", scenarioPath + "people.csv",
    scenarioPath + uniqueScenario + "_people.cache");
  Id firstLocationIdx = buildObjectLookupCache(numLocations,
    numLocationChares, scenarioPath + "locations.textproto",
    scenarioPath + "locations.csv", scenarioPath + uniqueScenario + "_locations.cache");
  buildActivityCache(numPeople, numDays, firstPersonIdx,
    scenarioPath + "visits.textproto", scenarioPath + "visits.csv",
    scenarioPath + uniqueScenario + "_visits.cache");
  return std::make_tuple(firstPersonIdx, firstLocationIdx, uniqueScenario);
}

Id buildObjectLookupCache(Id numObjs, PartitionId numChares,
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
  Id objPerChare = getNumElementsPerPartition(numObjs, numChares);

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

  // Special case to get lowest person ID first.
  std::getline(activityStream, line);
  char *str = strdup(line.c_str());
  char *tmp;
  char *tok = strtok_r(str, ",", &tmp);
  for (int i = 0; i < csvLocationOfPid; i++) {
    tok = strtok_r(tmp, ",", &tmp);
  }
  Id firstIdx = std::atoi(tok);
  free(str);
#if ENABLE_DEBUG >= DEBUG_VERBOSE
  CkPrintf("  Found first id as %d\n", firstIdx);
#endif

  // Check if file cache already created.
  std::ifstream existenceCheck(outputPath, std::ios_base::binary);
  if (existenceCheck.good()) {
    CkPrintf("Using existing cache.\n");
    existenceCheck.close();

  } else {
    existenceCheck.close();
    std::ofstream outputStream(outputPath, std::ios_base::binary);
    for (int chareNum = 0; chareNum < numChares; chareNum++) {
      // Write current offset.
      outputStream.write(reinterpret_cast<char *>(&currentPosition),
        sizeof(CacheOffset));

      // Skip next n lines.
      // We already read the first location on the first chare to get
      // its id, so don't double count that line
      Id numObjs = objPerChare - (0 == chareNum);
      for (int i = 0; i < numObjs; i++) {
        std::getline(activityStream, line);
      }
      currentPosition = activityStream.tellg();
    }
    outputStream.flush();
  }

  return firstIdx;
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
    CkPrintf("Activity cache already exists.");
    return;
  }

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
  CkPrintf("Parsed a total of %d visits\n", totalVisits);

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

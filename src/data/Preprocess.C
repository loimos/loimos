/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * This file is responsible for building a file cache pre-simulation to allow
 * direct fseeking in files rather than slow iteration. Three caches are built
 * for each of the people, locations, and activities file. The cache is dependent
 * on the files given and the number of chares 
 */ 

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <tuple>
#include <sstream>
#include <google/protobuf/text_format.h>

#include "../loimos.decl.h"
#include "Preprocess.h"
#include "../Person.h"

#define MAX_WRITE_SIZE 65536 // 2^16

/** 
 * This file preprocesses a given input file.
 */ 
// TODO: Replace getline with function that doesn't need to copy to string object.
std::tuple<int, int, std::string> buildCache(std::string scenarioPath, int numPeople, int peopleChares, int numLocations, int numLocationChares, int numDays) {
    // Build person and location cache.
    std::ostringstream oss;
    oss << numPeople << "-" << peopleChares << "_" << numLocations << "-" << numLocationChares;
    std::string unique_scenario = oss.str();
    int firstPersonIdx = buildObjectLookupCache(scenarioPath + "people.csv", scenarioPath + unique_scenario + "_people.cache", numPeople, peopleChares, scenarioPath + "people.textproto");
    int firstLocationIdx = buildObjectLookupCache(scenarioPath + "locations.csv", scenarioPath + unique_scenario + "_locations.cache", numLocations, numLocationChares, scenarioPath + "locations.textproto");
    buildActivityCache(scenarioPath + "interactions.csv", scenarioPath + unique_scenario + "_interactions.cache", numPeople, numDays, firstPersonIdx, scenarioPath + "interactions.textproto");
    return std::make_tuple(firstPersonIdx, firstLocationIdx, unique_scenario);
}

int buildObjectLookupCache(std::string inputPath, std::string outputPath, int numObjs, int numChares, std::string pathToCsvDefinition) {
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
    int objPerChare = numObjs / numChares;

    // Read config file.
    loimos::proto::CSVDefinition csvDefinition;
    std::ifstream t(pathToCsvDefinition);
    std::string strData((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    if (!google::protobuf::TextFormat::ParseFromString(strData, &csvDefinition)) {
        CkAbort("Could not parse protobuf!");
    }
    t.close();

    int csvLocationOfPid = -1;
    for (int i = 0; i < csvDefinition.field_size(); i += 1) {
        if (csvDefinition.field(i).has_uniqueid()) {
            csvLocationOfPid = i;
            break;
        }
    }
    assert(csvLocationOfPid != -1);

    std::string line;
    // Clear header.
    std::getline(activityStream, line);
    uint32_t currentPosition = activityStream.tellg();;

    // Special case to get lowest person ID first.
    std::getline(activityStream, line);
    char *str = strdup(line.c_str());
    char *tok = strtok(str, ",");
    for (int i = 0; i < csvLocationOfPid; i++) {
        tok = strtok(NULL, ",");
    }
    int firstIdx = std::atoi(tok);
    free(str);
        
    // Check if file cache already created.
    std::ifstream existenceCheck(outputPath, std::ios_base::binary);
    if (existenceCheck.good()) {
        printf("Using existing cache.\n");
        existenceCheck.close();
        return firstIdx;
    } else {
        existenceCheck.close();
        std::ofstream outputStream(outputPath, std::ios_base::binary);
        for (int chareNum = 0; chareNum < numChares; chareNum++) {
            // Write current offset.
            outputStream.write((char *) &currentPosition, sizeof(uint32_t));

            // Skip next n lines.
            for (int i = 0; i < objPerChare; i++) {
                std::getline(activityStream, line);
            }
            currentPosition = activityStream.tellg();
        }
        outputStream.flush();
        return firstIdx;
    }
}


void buildActivityCache(std::string inputPath, std::string outputPath, int numPeople, int numDays, int firstPersonIdx, std::string pathToCsvDefinition) {
    /**
     * Assumptions. 
     * Stream is sorted by start time per person.
     */
    // Check if cache already created.
    std::ifstream existenceCheck(outputPath, std::ios_base::binary);
    if (existenceCheck.good()) {
        printf("Activity cache already exists.");
        return;
    }

    std::ifstream activityStream(inputPath, std::ios_base::binary);
    if (!activityStream) {
        printf("Could not open person data input.\n");
        exit(1);
    }

    // Read config file.
    loimos::proto::CSVDefinition csvDefinition;
    std::ifstream t(pathToCsvDefinition);
    std::string strData((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    if (!google::protobuf::TextFormat::ParseFromString(strData, &csvDefinition)) {
        CkAbort("Could not parse protobuf!");
    }
    t.close();

    // Create position vector for each person.
    uint64_t totalDataSize = numPeople * numDays * sizeof(uint32_t);
    uint8_t *elements = (uint8_t *) malloc(totalDataSize);
    if (elements == NULL) {
        printf("Failed to malloc enoough memory for preprocessing.\n");
        exit(1);
    }
    memset(elements, 0xFF, totalDataSize);

    // Various initialization.
    std::string line;
    // Clear header.
    std::getline(activityStream, line);
    uint32_t current_position = activityStream.tellg();
    int lastPerson = -1;
    int lastTime = -1;
    int nextPerson = 0;
    int nextTime = 0;
    int personId = -1;
    int duration = -1;
    // For better looping efficiency simulate one break of inner loop to start.
    std::tie(nextPerson, personId, nextTime, duration) = DataReader<Person>::parseActivityStream(&activityStream, &csvDefinition, NULL);

    // Loop over the entire activity file and note boundaries on people and days
    while (!activityStream.eof()) {
        // Get details of new entry.
        lastPerson = nextPerson;
        lastTime = nextTime;
        memcpy(elements + sizeof(uint32_t) * (numDays * (lastPerson - firstPersonIdx) + lastTime),
               &current_position, sizeof(uint32_t));
    
        // Scan until the next boundary.
        while (!activityStream.eof() && lastTime == nextTime && lastPerson == nextPerson) {
            current_position = activityStream.tellg();
            std::tie(nextPerson, personId, nextTime, duration) = DataReader<Person>::parseActivityStream(&activityStream, &csvDefinition, NULL);
            nextTime = getDay(nextTime);
        }
    }

    // Output
    std::ofstream outputStream(outputPath);
    outputStream.write((char *) elements, totalDataSize);
    outputStream.close();
}

int getDay(int timeInSeconds) {
    return (timeInSeconds / (3600*24));
}

/* Copyright 2020 The Loimos Project Developers.
* See the top-level LICENSE file for details.
*
* SPDX-License-Identifier: MIT
*/

#include "../loimos.decl.h"
#include "../Defs.h"

#include <vector>
#include <stdio.h>
#include <string>
#include <fstream>

#include "data.pb.h"
#include "DataInterface.h"
#include "DataInterfaceMessage.h"
#include "../DiseaseModel.h"

DataLoader::DataLoader() {}

void DataLoader::BeginDataLoading() {
    // Calculate if this chare needs to load people of locations.
    int numPeopleLoadingChares = (numPeoplePartitions + LOADING_CHARES_PER_CHARE - 1) / LOADING_CHARES_PER_CHARE;
    bool personReader = thisIndex < numPeopleLoadingChares;
    // Determine relative position among people or data loaders.
    int objOfType = personReader ? numPeople : numLocations;
    int charesOfType = personReader ? numPeoplePartitions : numLocationPartitions;
    int relIndex = personReader ? thisIndex : thisIndex - numPeopleLoadingChares;
    // Calculate range of chares.
    int lowerChare = relIndex * LOADING_CHARES_PER_CHARE;
    int upperChare = std::min(charesOfType - 1, (relIndex + 1) * LOADING_CHARES_PER_CHARE - 1);
    
    // Open relevant files.
    const DiseaseModel *diseaseModel = globDiseaseModel.ckLocalBranch();
    std::string dataPath = scenarioPath
                        + (personReader ? "people.csv" : "locations.csv");
    std::ifstream dataStream(dataPath);
    std::string cachePath = scenarioPath + scenarioId
                        + (personReader ? "_people.cache" : "_locations.cache");
    std::ifstream cacheStream(cachePath);
    if (!dataStream || !cacheStream) {
        CkAbort("Could not open person data input.");
    }

    // Create object to read into.
    int numAttributesPerObj = personReader ? 
        DataReader::getNumberOfDataAttributes(diseaseModel->personDef) :
        DataReader::getNumberOfDataAttributes(diseaseModel->locationDef); 

    // Read in data for each chare sequentially.
    int firstIdx = personReader ? firstPersonIdx : firstLocationIdx;
    for (int targetChare = lowerChare; targetChare <= upperChare; targetChare++) {
        // Seek to correct position in cache.
        cacheStream.seekg(targetChare * sizeof(uint32_t));
        uint32_t offset;
        cacheStream.read((char *) &offset, sizeof(uint32_t));
        // Seek to start of data in stream.
        dataStream.seekg(offset);

        // Calculate range of people.
        // Load in location information.
        int numElements = getNumLocalElements(
            objOfType,
            charesOfType,
            targetChare
        );

        for (int i = 0; i < numElements; i++) {
            // Message freed by system.
            DataInterfaceMessage *msg = new DataInterfaceMessage(numAttributesPerObj);
            assert(msg != NULL);

            if (personReader) {
                DataReader::readData(&dataStream, diseaseModel->personDef, msg);
                peopleArray[targetChare].ReceivePersonSetup(msg);
            } else {
                DataReader::readData(&dataStream, diseaseModel->locationDef, msg);
                locationsArray[targetChare].ReceiveLocationSetup(msg);
            }
        }
    }

    // Cleanup.
    dataStream.close();
    cacheStream.close();
}


void DataLoader::readData(std::ifstream *input, loimos::proto::CSVDefinition *dataFormat, DataInterfaceMessage *msg) {
    // TODO make this 2^16 and support longer lines through multiple reads.
    char buf[MAX_INPUT_lineLength];
    // Get next line.
    input->getline(buf, MAX_INPUT_lineLength);

    // Read over people data format.
    int attrIndex = 0;
    // Tracks how many non-ignored fields there have been.
    int numDataFields = 0;
    int leftCommaLocation = 0;
    union Data *objData = (union Data *) msg->dataAttributes;

    int lineLength = input->gcount();
    for (int c = 0; c < lineLength; c++) {
        // Scan for the next attrbiutes - comma separted.
        if (buf[c] == CSV_DELIM || c + 1 == lineLength) {
            // Get next attribute type.
            loimos::proto::Data_Field const *field = &dataFormat->field(attrIndex);
            uint16_t dataLen = c - leftCommaLocation;
            if (field->has_ignore() || dataLen == 0) {
                // Skip
            } else {
                // Process data.
                char *start = buf + leftCommaLocation;
                if (c + 1 == lineLength) {
                    dataLen += 1;
                }

                if (field->has_uniqueid()) {
                    msg->uniqueId = std::stoi(std::string(start, dataLen));
                } else {
                    // Parse byte stream to the correct representation.
                    if (field->has_b10int() || field->has_foreignid()) {
                        // TODO parse this directly.
                        objData[numDataFields].int_b10 = 
                            std::stoi(std::string(start, dataLen));
                    } else if (field->has_label()) {
                        // objData[numDataFields].str = 
                            // new std::string(start, dataLen);
                    } else if (field->has_bool_()) {
                        if (dataLen == 1) {
                            objData[numDataFields].boolean = 
                            (start[0] == 't' || start[0] == '1');
                        } else {
                            objData[numDataFields].boolean = false;
                        }
                    }
                    numDataFields++;
                }
            }
            leftCommaLocation = c + 1;
            attrIndex++;
        }
    }
}
static int DataLoader::getNumberOfDataAttributes(loimos::proto::CSVDefinition *dataFormat) {
    int count = 0;
    for (int c = 0; c < dataFormat->field_size(); c++) {
        if (!dataFormat->field(c).has_ignore() && !dataFormat->field(c).has_uniqueid()) {
            count += 1;
        }
    }
    return count;
}

static std::tuple<int, int, int, int> DataLoader::parseActivityStream(std::ifstream *input, loimos::proto::CSVDefinition *dataFormat, std::vector<union Data> *attributes) {
    int personId = -1;
    int locationId = -1;
    int startTime = -1;
    int duration = -1;
    char buf[MAX_INPUT_lineLength];

    // Get header line.
    input->getline(buf, MAX_INPUT_lineLength);

    // Read over people data format.
    int attrIndex = 0;
    int numDataFields = 0;
    int leftCommaLocation = 0;

    int lineLength = input->gcount();
    for (int c = 0; c < lineLength; c++) {
        // Scan for the next attrbiutes - comma separted.
        if (buf[c] == CSV_DELIM || c + 1 == lineLength) {
            // Get next attribute type.
            loimos::proto::Data_Field const *field = &dataFormat->field(attrIndex);
            uint16_t dataLen = c - leftCommaLocation;
            if (field->has_ignore() || dataLen == 0) {
                // Skip
            } else {
                // Process data.
                char *start = buf + leftCommaLocation;
                if (c + 1 == lineLength) {
                    dataLen += 1;
                } else {
                    start[dataLen] = 0;
                }

                // Parse byte stream to the correct representation.
                if (field->has_uniqueid()) {
                    personId = std::atoi(start);
                } else if (field->has_foreignid()) {
                    locationId = std::atoi(start);
                } else if (field->has_starttime()) {
                    startTime = std::atoi(start);
                } else if (field->has_duration()) {
                    duration = std::atoi(start);
                }
                numDataFields++;                
            }
            leftCommaLocation = c + 1;
            attrIndex++;
        }
    }

    return std::make_tuple(personId, locationId, startTime, duration);
}
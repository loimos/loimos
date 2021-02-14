/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __DATA_READER_H__
#define __DATA_READER_H__
#include <vector>
#include <stdio.h>
#include <string>
#include <fstream>

#include "data.pb.h"
#include "DataInterface.h"

#define MAX_INPUT_lineLength (std::streamsize) 262144 // 2^18

namespace DataTypes {
    enum DataType { int_b10, uint_32, string, category }; 
}

union Data {
    int int_b10;
    bool boolean;
    uint32_t uint_32;
    std::string *str;
    uint16_t category; 
};


/**
 * Defines a generic data reader for any child class of DataInterface.
 * Array definition is child object dependent so this required that the
 * code be defined in the .h file rather than in the .C.
 */ 
template <class T>
class DataReader {
    public:
        static void readData(std::ifstream *input, loimos::proto::CSVDefinition *dataFormat,
                    std::vector<T> *dataObjs) {
            // TODO make this 2^16 and support longer lines through multiple reads.
            char buf[MAX_INPUT_lineLength];
            // Rows to read.
            for (auto obj = std::begin(*dataObjs); obj != std::end(*dataObjs); ++obj) {
                // Get next line.
                input->getline(buf, MAX_INPUT_lineLength);

                // Read over people data format.
                int attrIndex = 0;
                // Tracks how many non-ignored fields there have been.
                int numNonIgnoredFields = 0;
                int leftCommaLocation = 0;
                std::vector<union Data> objData = obj->getDataField();

                int lineLength = input->gcount();
                for (int c = 0; c < lineLength; c++) {
                    // Scan for the next attrbiutes - comma separted.
                    if (buf[c] == CSV_DELIM || c + 1 == lineLength) {
                        // Get next attribute type.
                        loimos::proto::Data_Field const *field = &dataFormat->field(attrIndex);
                        uint16_t dataLen = c - leftCommaLocation;
                        if (field->has_ignore() || dataLen == 0) {
                            // Skip
                        } else if (numNonIgnoredFields <= 3) {
                            // Process data.
                            char *start = buf + leftCommaLocation;
                            if (c + 1 == lineLength) {
                                dataLen += 1;
                            }

                            // Parse byte stream to the correct representation.
                            if (field->has_uniqueid()) {
                                obj->setUniqueId(std::stoi(std::string(start, dataLen)));
                            } else if (field->has_b10int() || field->has_foreignid()) {
                                // TODO parse this directly.
                                objData[numNonIgnoredFields].int_b10 = 
                                    std::stoi(std::string(start, dataLen));
                            } else if (field->has_label()) {
                                objData[numNonIgnoredFields].str = 
                                    new std::string(start, dataLen);
                            } else if (field->has_bool_()) {
                                if (dataLen == 1) {
                                    objData[numNonIgnoredFields].boolean = 
                                    (start[0] == 't' || start[0] == '1');
                                } else {
                                    objData[numNonIgnoredFields].boolean = false;
                                }
                            }
                            numNonIgnoredFields++;
                        }
                        leftCommaLocation = c + 1;
                        attrIndex++;
                    }
                }
            }
        }
        static int getNonZeroAttributes(loimos::proto::CSVDefinition *dataFormat) {
            int count = 0;
            for (int c = 0; c < dataFormat->field_size(); c++) {
                if(!dataFormat->field(c).has_ignore()) {
                    count += 1;
                }
            }
            return count;
        }

        static std::tuple<int, int, int, int> parseActivityStream(std::ifstream *input, loimos::proto::CSVDefinition *dataFormat, std::vector<union Data> *attributes) {
            int personId = -1;
            int locationId = -1;
            int startTime = -1;
            int duration = -1;
            // TODO don't reallocate this everytime.
            char buf[MAX_INPUT_lineLength];

            // Get header line.
            input->getline(buf, MAX_INPUT_lineLength);

            // Read over people data format.
            int attrIndex = 0;
            int numNonIgnoredFields = 0;
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
                    } else if (numNonIgnoredFields <= 3) {
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
                        } else {
                            // TODO process.
                            numNonIgnoredFields++;
                        }
                        
                    }
                    leftCommaLocation = c + 1;
                    attrIndex++;
                }
            }
            // printf("Person %d visited %d at %d for %d\n", personId, locationId, startTime, duration);
            return std::make_tuple(personId, locationId, startTime, duration);
        }
};
#endif

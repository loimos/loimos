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

#define MAX_INPUT_LINE_LENGTH (std::streamsize) 262144 // 2^18

namespace DataTypes {
    enum DataType { int_b10, uint_32, string, category }; 
}

union Data {
    int int_b10;
    bool boolean;
    uint32_t uint_32;
    std::string str;
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
            char buf[MAX_INPUT_LINE_LENGTH];
            // Rows to read.
            for(T obj : *dataObjs) {
                // Get next line.
                input->getline(buf, MAX_INPUT_LINE_LENGTH);

                // Read over people data format.
                int attr_index = 0;
                int attr_nonzero_index = 0;
                int left_comma = 0;
                union Data *obj_data = obj.getDataField();

                int line_length = input->gcount();
                for (int c = 0; c < line_length; c++) {
                    // Scan for the next attrbiutes - comma separted.
                    if (buf[c] == ',' || c + 1 == line_length) {
                        // Get next attribute type.
                        loimos::proto::Data_Field const *field = &dataFormat->field(attr_index);
                        uint16_t data_len = c - left_comma;
                        if (field->has_ignore() || data_len == 0) {
                            // Skip
                        } else if (attr_nonzero_index <= 3) {
                            // Process data.
                            char *start = buf + left_comma;
                            if (c + 1 == line_length) {
                                data_len += 1;
                            }

                            // Parse byte stream to the correct representation.
                            if (field->has_uniqueid()) {
                                obj.setUniqueId(std::stoi(std::string(start, data_len)));
                            } else if (field->has_b10int() || field->has_foreignid()) {
                                // TODO parse this directly.
                                obj_data[attr_nonzero_index].int_b10 = 
                                    std::stoi(std::string(start, data_len));
                            } else if (field->has_label()) {
                                obj_data[attr_nonzero_index].str = 
                                    std::string(start, data_len);
                            } else if (field->has_bool_()) {
                                if (data_len == 1) {
                                    obj_data[attr_nonzero_index].boolean = 
                                    (start[0] == 't' || start[0] == '1');
                                } else {
                                    obj_data[attr_nonzero_index].boolean = false;
                                }
                            }
                            attr_nonzero_index++;
                        }
                        left_comma = c + 1;
                        attr_index++;
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

        static std::tuple<int, int, int, int> parseActivityStream(std::ifstream *input, loimos::proto::CSVDefinition *dataFormat, union Data *attributes) {
            int personId = -1;
            int locationId = -1;
            int startTime = -1;
            int duration = -1;
            // TODO don't reallocate this everytime.
            char buf[MAX_INPUT_LINE_LENGTH];

            // Get header line.
            input->getline(buf, MAX_INPUT_LINE_LENGTH);

            // Read over people data format.
            int attr_index = 0;
            int attr_nonzero_index = 0;
            int left_comma = 0;

            int line_length = input->gcount();
            for (int c = 0; c < line_length; c++) {
                // Scan for the next attrbiutes - comma separted.
                if (buf[c] == ',' || c + 1 == line_length) {
                    // Get next attribute type.
                    loimos::proto::Data_Field const *field = &dataFormat->field(attr_index);
                    uint16_t data_len = c - left_comma;
                    if (field->has_ignore() || data_len == 0) {
                        // Skip
                    } else if (attr_nonzero_index <= 3) {
                        // Process data.
                        char *start = buf + left_comma;
                        if (c + 1 == line_length) {
                            data_len += 1;
                        } else {
                            start[data_len] = 0;
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
                            attr_nonzero_index++;
                        }
                        
                    }
                    left_comma = c + 1;
                    attr_index++;
                }
            }
            // printf("Person %d visited %d at %d for %d\n", personId, locationId, startTime, duration);
            return std::make_tuple(personId, locationId, startTime, duration);
        }
};
#endif
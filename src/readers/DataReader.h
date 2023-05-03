/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef READERS_DATAREADER_H_
#define READERS_DATAREADER_H_

#include "../Defs.h"
#include "data.pb.h"
#include "DataInterface.h"

#include <vector>
#include <stdio.h>
#include <string>
#include <fstream>
#include <tuple>

#define MAX_INPUT_lineLength (std::streamsize) 262144  // 2^18

/**
 * Defines a generic data reader for any child class of DataInterface.
 * Array definition is child object dependent so this required that the
 * code be defined in the .h file rather than in the .C.
 */
template <class T = DataInterface>
class DataReader {
 public:
  static void readData(std::ifstream *input,
      loimos::proto::CSVDefinition *dataFormat,
      std::vector<T> *dataObjs) {
    char buf[MAX_INPUT_lineLength];
    // Rows to read.
    for (T &obj : *dataObjs) {
      // Get next line.
      input->getline(buf, MAX_INPUT_lineLength);

      // Read over people data format.
      int attrIndex = 0;
      // Tracks how many non-ignored fields there have been.
      int numDataFields = 0;
      int leftCommaLocation = 0;

      int lineLength = input->gcount();
      for (int c = 0; c < lineLength; c++) {
        // Scan for the next attributes - comma separated.
        if (buf[c] != CSV_DELIM && c + 1 != lineLength) {
          continue;
        }

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
          std::string rawData(start, dataLen);
          numDataFields +=
            DataReader<T>::parseObjectData(rawData, field, numDataFields, &obj);
        }

        leftCommaLocation = c + 1;
        attrIndex++;
      }
    }
  }

  // Returns 1 if another field was set in the data vector of obj
  // and 0 otherwise
  static int parseObjectData(const std::string &rawData,
      const loimos::proto::Data_Field *field, int fieldIdx, T *obj) {
    std::vector<union Data> *data = &obj->getData();

    // Parse byte stream to the correct representation.
    if (field->has_uniqueid()) {
      obj->setUniqueId(std::stoi(rawData));
      return 0;
    } else {
      if (field->has_b10int() || field->has_foreignid()) {
        data->at(fieldIdx).int_b10 = std::stoi(rawData);

      } else if (field->has_label()) {
        data->at(fieldIdx).str = new std::string(rawData);

      } else if (field->has_bool_()) {
        if (rawData.length() == 1) {
          data->at(fieldIdx).boolean =
            (rawData[0] == 't' || rawData[0] == '1');
        } else {
          data->at(fieldIdx).boolean = false;
        }
      }

      return 1;
    }
  }

  static std::tuple<int, int, int, int> parseActivityStream(std::ifstream *input,
      loimos::proto::CSVDefinition *dataFormat, std::vector<union Data> *attributes) {
    int personId = -1;
    int locationId = -1;
    int startTime = -1;
    int duration = -1;
    // TODO(IanCostello) don't reallocate this every time.
    char buf[MAX_INPUT_lineLength];

    // Get header line.
    input->getline(buf, MAX_INPUT_lineLength);

    // Read over people data format.
    int attrIndex = 0;
    int numDataFields = 0;
    int leftCommaLocation = 0;

    int lineLength = input->gcount();
    for (int c = 0; c < lineLength; c++) {
      // Scan for the next attributes - comma separated.
      if (buf[c] == CSV_DELIM || c + 1 == lineLength) {
        // Get next attribute type.
        loimos::proto::Data_Field const *field = &dataFormat->field(attrIndex);
        uint16_t dataLen = c - leftCommaLocation;
        if (field->has_ignore() || dataLen == 0) {
          // Skip
        } else if (numDataFields <= 3) {
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
            numDataFields++;
          }
        }
        leftCommaLocation = c + 1;
        attrIndex++;
      }
    }
    return std::make_tuple(personId, locationId, startTime, duration);
  }

  static int getNonZeroAttributes(loimos::proto::CSVDefinition *dataFormat) {
    int count = 0;
    for (int c = 0; c < dataFormat->field_size(); c++) {
      if (!dataFormat->field(c).has_ignore()) {
        count += 1;
      }
    }
    return count - 1;
  }
};
#endif  // READERS_DATAREADER_H_

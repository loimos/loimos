/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef READERS_DATAREADER_H_
#define READERS_DATAREADER_H_

#include "DataInterface.h"
#include "../protobuf/data.pb.h"
#include "../Defs.h"

#include <vector>
#include <stdio.h>
#include <string>
#include <fstream>
#include <tuple>

#define MAX_INPUT_lineLength (std::streamsize) 262144  // 2^18
#define CSV_DELIM ','

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
      CacheOffset pos = input->tellg();
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
        loimos::proto::DataField const *field = &dataFormat->fields(attrIndex);
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
          try {
            numDataFields +=
              DataReader<T>::parseObjectData(rawData, field, numDataFields, &obj);
          } catch (const std::exception &e) {
            CkPrintf("Error at byte %lu: '%s' (%s)\n", pos, buf, rawData.c_str());
            CkAbort("%\n", e.what());
          }
        }

        leftCommaLocation = c + 1;
        attrIndex++;
      }
    }
  }

  // Returns 1 if another field was set in the data vector of obj
  // and 0 otherwise
  static int parseObjectData(const std::string &rawData,
      const loimos::proto::DataField *field, int fieldIdx, T *obj) {
    std::vector<union Data> *data = &obj->getData();

    // Parse byte stream to the correct representation.
    if (field->has_unique_id()) {
      obj->setUniqueId(ID_PARSE(rawData));
      return 0;
    } else {
      if (field->has_int32()) {
        data->at(fieldIdx).int32_val = std::stoi(rawData);

      } else if (field->has_int64()) {
        data->at(fieldIdx).int64_val = std::stol(rawData);
      
      } else if (field->has_uint32()) {
        data->at(fieldIdx).uint32_val =
          static_cast<uint32_t>(std::stoi(rawData));

      } else if (field->has_uint64()) {
        data->at(fieldIdx).uint64_val =
          static_cast<uint64_t>(std::stol(rawData));

      } else if (field->has_foreign_id()) {
        data->at(fieldIdx).CONCAT(ID_PROTOBUF_TYPE, _val) =
          ID_PARSE(rawData);

      } else if (field->has_double_()) {
        data->at(fieldIdx).double_val = std::stod(rawData);

      } else if (field->has_string()) {
        data->at(fieldIdx).string_val = new std::string(rawData);

      } else if (field->has_bool_()) {
        data->at(fieldIdx).bool_val = (rawData.length() == 1)
          && (rawData[0] == 't' || rawData[0] == '1');
      }

      return 1;
    }
  }

  static std::tuple<Id, Id, Time, Time> parseActivityStream(std::ifstream *input,
      loimos::proto::CSVDefinition *dataFormat, std::vector<union Data> *attributes) {
    Id personId = -1;
    Id locationId = -1;
    Time startTime = -1;
    Time duration = -1;
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
        loimos::proto::DataField const *field = &dataFormat->fields(attrIndex);
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
          if (field->has_unique_id()) {
            personId = ID_PARSE(start);

          } else if (field->has_foreign_id()) {
            locationId = ID_PARSE(start);

          } else if (field->has_start_time()) {
            startTime = TIME_PARSE(start);

          } else if (field->has_duration()) {
            duration = TIME_PARSE(start);

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
    int count = -1;
    for (uint c = 0; c < dataFormat->fields_size(); c++) {
      if (!dataFormat->fields(c).has_ignore()) {
        count++;
      }
    }
    return count;
  }

  static int getAttributeIndex(loimos::proto::CSVDefinition *dataFormat,
      std::string attributeName) {
    int count = -1;
    for (uint i = 0; i < dataFormat->fields_size(); ++i) {
      loimos::proto::DataField const *field = &dataFormat->fields(i);

      // Only need to keep track of location among attributes stored in
      // data vector
      if (field->has_ignore() || field->has_unique_id()
          || field->has_foreign_id()) {
        continue;
      }

      count++;
      if (field->field_name() == attributeName) {
        return count;
      }
    }

    return -1;
  }
};
#endif  // READERS_DATAREADER_H_

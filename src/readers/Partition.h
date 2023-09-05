/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef READERS_PARTITION_H_
#define READERS_PARTITION_H_

#include "DataInterface.h"
#include "DataReader.h"
#include "../protobuf/data.pb.h"
#include "../Defs.h"
#include "../Types.h"

#include <string>
#include <vector>

template <class T = DataInterface>
void partitionData(Id numObjs, PartitionId numChares,
    std::string metadataPath, std::string inputPath, std::string outputPath,
    std::vector<Id> *lidUpdate) {
}

void updateVisitIds(const std::vector<Id> &lidUpdate,
  const std::vector<Id> &pidUpdate, std::string metadataPath,
  std::string inputPath, std::string outputPath) {

}

#endif  // READERS_PARTITION_H_
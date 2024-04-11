/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef PARTITIONER_H__
#define PARTITIONER_H__

#include "Types.h"
#include "protobuf/data.pb.h"

#include <vector>
#include <string>

template <typename T>
bool outOfBounds(T lower, T upper, T value) {
  return lower > value || upper <= value;
}

// Previous, completely algebraic, paritioning scheme:
// Some functions are still in use in the new scheme but most should be
// consider depricated outside that scope
Id getNumElementsPerPartition(Id numElements, PartitionId numPartitions);
PartitionId getNumLargerPartitions(Id numElements, PartitionId numPartitions);
PartitionId getPartitionIndex(Id globalIndex, Id numElements,
    PartitionId numPartitions, Id offset);
Id getFirstIndex(PartitionId partitionIndex, Id numElements,
    PartitionId numPartitions, Id offset);
Id getNumLocalElements(Id numElements, PartitionId numPartitions,
    PartitionId partitionIndex);
Id getGlobalIndex(Id localIndex, PartitionId partitionIndex, Id numElements,
    PartitionId numPartitions, Id offset);
Id getLocalIndex(Id globalIndex, PartitionId partitionIndex, Id numElements,
    PartitionId numPartitions, Id offset);

// New, offset-based partitioning scheme:
Id getLocalIndex(Id globalIndex, PartitionId PartitionId,
    const std::vector<Id> &offsets);
Id getGlobalIndex(Id localIndex, PartitionId PartitionId,
    const std::vector<Id> &offsets);
PartitionId getPartition(Id globalIndex, const std::vector<Id> &offsets);
Id getPartitionSize(PartitionId partitionIndex,
    Id numObjects, const std::vector<Id> &offsets);

struct Partitioner {
  Id numPeople;
  Id numLocations;
  std::vector<Id> locationPartitionOffsets;
  std::vector<Id> personPartitionOffsets;

  Partitioner(std::string scenarioPath,
    PartitionId numPersonPartitions,
    PartitionId numLocationPartitions,
    loimos::proto::CSVDefinition *personMetadata,
    loimos::proto::CSVDefinition *locationMetadata);
  Partitioner(PartitionId numPersonPartitions,
    PartitionId numLocationPartitions, Id numPeople,
    Id numLocations);

  static void setPartitionOffsets(PartitionId numPartitions,
    Id firstIndex, Id numObjects, loimos::proto::CSVDefinition *metadata,
    std::vector<Id> *partitionOffsets);
  static void setPartitionOffsets(PartitionId numPartitions,
    Id firstIndex, Id numObjects, std::vector<Id> *partitionOffsets);

  // Offset-based index interface - each calls the corresponding function
  // with the corresponding offset vector
  Id getLocalLocationIndex(Id globalIndex, PartitionId PartitionId) const;
  Id getGlobalLocationIndex(Id localIndex, PartitionId PartitionId) const;
  CacheOffset getLocationCacheIndex(Id globalIndex) const;
  PartitionId getLocationPartitionIndex(Id globalIndex) const;
  Id getLocationPartitionSize(PartitionId partitionIndex) const;
  PartitionId getNumLocationPartitions();

  Id getLocalPersonIndex(Id globalIndex, PartitionId PartitionId) const;
  Id getGlobalPersonIndex(Id localIndex, PartitionId PartitionId) const;
  CacheOffset getPersonCacheIndex(Id globalIndex) const;
  PartitionId getPersonPartitionIndex(Id globalIndex) const;
  Id getPersonPartitionSize(PartitionId partitionIndex) const;
  PartitionId getNumPersonPartitions();
};

#endif  // PARTITIONER_H__

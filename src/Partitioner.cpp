/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "charm++.h"
#include "Partitioner.h"
#include "Defs.h"
#include "Types.h"
#include "protobuf/data.pb.h"
#include "readers/Preprocess.h"

#include <vector>
#include <cmath>
#include <algorithm>
#include <iterator>
#include <string>

/**
 * Returns the number of elements that each chare will track at a maximum
 *
 * Args:
 *    Id numElements: The total number of objects of this type.
 *    Id numPartitions: The total number of chares.
 *
 */
Id getNumElementsPerPartition(Id numElements, PartitionId numPartitions) {
  return static_cast<Id>(ceil(1.0 * numElements / numPartitions));
}

/**
 * Returns the number of chares that will which will have the maximum number
 * of elements (as returned by getNumElementsPerPartition). All other chares
 * will have one less element.
 *
 * Args:
 *    Id numElements: The total number of objects of this type.
 *    Id numPartitions: The total number of chares.
 *
 */
PartitionId getNumLargerPartitions(Id numElements, PartitionId numPartitions) {
  Id numLargerPartitions = numElements % numPartitions;
  if (0 == numLargerPartitions) {
    numLargerPartitions = numPartitions;
  }
  return numLargerPartitions;
}

/**
 * Returns the number of the chare that tracks a particular element.
 *
 * Args:
 *    Id globalIndex: The global unique object identifier.
 *    Id numElements: The total number of objects of this type.
 *    Id numPartitions: The total number of chares.
 *    Id offset: The globalIndex number referring to the first object. Likely
 *      could be replaced with a better system.
 *
 * Notes: Will likely change as we introduce active load balancing.
 */
PartitionId getPartitionIndex(Id globalIndex, Id numElements,
    PartitionId numPartitions, Id offset) {
  globalIndex -= offset;

  Id elementsPerPartition = getNumElementsPerPartition(numElements,
      numPartitions);

  Id numLargerPartitions = getNumLargerPartitions(numElements, numPartitions);
  Id numElementsInLargerPartitions =
    numLargerPartitions * elementsPerPartition;

  if (globalIndex < numElementsInLargerPartitions) {
    return globalIndex / elementsPerPartition;
  } else {
    return numLargerPartitions - 1
      + (globalIndex - numElementsInLargerPartitions)
      / (elementsPerPartition - 1);
  }
}

/**
 * Returns the first global index of an object present on the specified chare
 *
 * Args:
 *    Id partitionIndex: The index of the chare in question
 *    Id numElements: The total number of objects of this type.
 *    Id numPartitions: The total number of chares.
 *    Id offset: The globalIndex number referring to the first object. Likely
 *      could be replaced with a better system.
 *
 */
Id getFirstIndex(PartitionId partitionIndex, Id numElements,
    PartitionId numPartitions, Id offset) {
  Id elementsPerPartition = getNumElementsPerPartition(numElements,
      numPartitions);
  Id maxIndex = partitionIndex * elementsPerPartition + offset;

  PartitionId numLargerPartitions = getNumLargerPartitions(numElements,
      numPartitions);
  if (partitionIndex < numLargerPartitions) {
    return maxIndex;
  } else {
    return maxIndex - (partitionIndex - numLargerPartitions);
  }
}

/**
 * Returns the number of the objects that a particular chare tracks.
 *
 * Args:
 *    Id numElements: The total number of objects of this type.
 *    Id numPartitions: The total number of chares.
 *    Id partitionIndex: Chare index
 *
 */
Id getNumLocalElements(Id numElements, PartitionId numPartitions,
    PartitionId partitionIndex) {
  Id elementsPerPartition = getNumElementsPerPartition(numElements,
      numPartitions);
  Id numLargerPartitions = numElements % numPartitions;
  if (partitionIndex < numLargerPartitions) {
    return elementsPerPartition;
  } else {
    return elementsPerPartition - 1;
  }
}

/**
 * Returns global indexed position of an object from its local (zero-indexed)
 * id on a chare.
 *
 * Args:
 *    Id localIndex: The local (zero-indexed) unique object identifier.
 *    Id partitionIndex: The index of the chare containing the specified
 *      object
 *    Id numElements: The total number of objects of this type.
 *    Id numPartitions: The total number of chares.
 *    Id offset: The globalIndex number referring to the first object. Likely
 *      could be replaced with a better system.
 *
 */
Id getGlobalIndex(Id localIndex, PartitionId partitionIndex, Id numElements,
    PartitionId numPartitions, Id offset) {
  Id firstLocalIndex = getFirstIndex(partitionIndex, numElements,
      numPartitions, offset);
  return firstLocalIndex + localIndex;
}

/**
 * Returns local (zero-indexed) position of an object on a chare from its
 * global id.
 *
 * Args:
 *    Id globalIndex: The global unique object identifier.
 *    Id partitionIndex: The index of the chare containing the specified
 *      objectsrc/Defs.cpp
 *    Id numElements: The total number of objects of this type.
 *    Id numPartitions: The total number of chares.
 *    Id offset: The globalIndex number referring to the first object. Likely
 *      could be replaced with a better system.
 *
 */
Id getLocalIndex(Id globalIndex, PartitionId partitionIndex, Id numElements,
    PartitionId numPartitions, Id offset) {
  Id firstLocalIndex = getFirstIndex(partitionIndex, numElements,
      numPartitions, offset);
  return globalIndex - firstLocalIndex;
}

Id getLocalIndex(Id globalIndex, PartitionId PartitionId,
    const std::vector<Id> &offsets) {
  return globalIndex - offsets[PartitionId];
}

Id getGlobalIndex(Id localIndex, PartitionId PartitionId,
    const std::vector<Id> &offsets) {
  return localIndex + offsets[PartitionId];
}

PartitionId getPartition(Id globalIndex,
    const std::vector<Id> &offsets) {
  PartitionId result = std::distance(offsets.begin(),
    std::upper_bound(offsets.begin(), offsets.end(), globalIndex)) - 1;
  // CkPrintf("    Index " ID_PRINT_TYPE " in partition %d\n",
  //   globalIndex, result);
  return result;
}

Id getPartitionSize(PartitionId partitionIndex,
    Id numObjects, const std::vector<Id> &offsets) {
  if (offsets.size() - 1 == partitionIndex) {
    return numObjects + offsets[0] - offsets[partitionIndex];
  } else {
    return offsets[partitionIndex + 1] - offsets[partitionIndex];
  }
}

Partitioner::Partitioner(std::string scenarioPath,
    PartitionId numPersonPartitions,
    PartitionId numLocationPartitions,
    loimos::proto::CSVDefinition *personMetadata,
    loimos::proto::CSVDefinition *locationMetadata) :
    numPeople(personMetadata->num_rows()),
    numLocations(locationMetadata->num_rows()) {
  Id firstLocationIdx = getFirstIndex(locationMetadata, scenarioPath + "locations.csv");
  Id firstPersonIdx = getFirstIndex(personMetadata, scenarioPath + "people.csv");

  PartitionId numOffsets = personMetadata->partition_offsets_size();
  if (0 < numOffsets && numPersonPartitions > numOffsets) {
    CkAbort("Error: attempting to run with more person partitions ("
      PARTITION_ID_PRINT_TYPE ") than provided offsets ("
      PARTITION_ID_PRINT_TYPE ")\n",
      numPersonPartitions, numOffsets);
  }
  numOffsets = locationMetadata->partition_offsets_size();
  if (0 < numOffsets && numLocationPartitions > numOffsets) {
    CkAbort("Error: attempting to run with more location partitions ("
      PARTITION_ID_PRINT_TYPE ") than provided offsets ("
      PARTITION_ID_PRINT_TYPE ")\n",
      numLocationPartitions, numOffsets);
  }

  setPartitionOffsets(numPersonPartitions, firstPersonIdx, numPeople,
    personMetadata, &personPartitionOffsets);
  setPartitionOffsets(numLocationPartitions, firstLocationIdx, numLocations,
    locationMetadata, &locationPartitionOffsets);

#if ENABLE_DEBUG >= DEBUG_PER_CHARE
  if (0 == CkMyNode()) {
    for (int i = 0; i < personPartitionOffsets.size(); ++i) {
      CkPrintf("  Person Offset %d: " ID_PRINT_TYPE "\n",
        i, personPartitionOffsets[i]);
    }
    for (int i = 0; i < locationPartitionOffsets.size(); ++i) {
      CkPrintf("  Location Offset %d: " ID_PRINT_TYPE "\n",
        i, locationPartitionOffsets[i]);
    }
  }
#endif
}

Partitioner::Partitioner(PartitionId numPersonPartitions,
    PartitionId numLocationPartitions, Id numPeople_,
    Id numLocations_) :
    numPeople(numPeople_), numLocations(numLocations_) {
  setPartitionOffsets(numPersonPartitions, 0, numPeople,
    &personPartitionOffsets);
  setPartitionOffsets(numLocationPartitions, 0, numLocations,
    &locationPartitionOffsets);
}

void Partitioner::setPartitionOffsets(PartitionId numPartitions,
    Id firstIndex, Id numObjects, loimos::proto::CSVDefinition *metadata,
    std::vector<Id> *partitionOffsets) {
  partitionOffsets->reserve(numPartitions);
  Id lastIndex = firstIndex + numObjects;

  if (0 < metadata->partition_offsets_size()) {
    PartitionId numOffsets = metadata->partition_offsets_size();
    for (PartitionId i = 0; i < numPartitions; ++i) {
      PartitionId offsetIdx = getFirstIndex(i, numOffsets, numPartitions, 0);
      Id offset = metadata->partition_offsets(offsetIdx);
      partitionOffsets->emplace_back(offset);

#ifdef ENABLE_DEBUG
      if (outOfBounds(firstIndex, lastIndex, offset)) {
        CkAbort("Error: Offset " ID_PRINT_TYPE " outside of valid range [0,"
          ID_PRINT_TYPE")\n", offset, numObjects);

      // Offsets should be sorted so we can do a binary search later
      } else if (0 != i && partitionOffsets->at(i - 1) > offset) {
        CkAbort("Error: Offset " ID_PRINT_TYPE " (%d-th offset) for chare "
        PARTITION_ID_PRINT_TYPE" out of order\n", offset, offsetIdx, i);
      }
#endif  // ENABLE_DEBUG
    }

  } else {
    Partitioner::setPartitionOffsets(numPartitions, firstIndex, numObjects,
      partitionOffsets);
  }
}

// If no offsets are provided, try to put about the same number of objects
// in each partition (i.e. use the old partitioning scheme)
void Partitioner::setPartitionOffsets(PartitionId numPartitions,
    Id firstIndex, Id numObjects, std::vector<Id> *partitionOffsets) {
  for (PartitionId p = 0; p < numPartitions; ++p) {
    Id offset = getFirstIndex(p, numObjects,
        numPartitions, firstIndex);
    partitionOffsets->emplace_back(offset);

#ifdef ENABLE_DEBUG
    if (outOfBounds(firstIndex, numObjects, offset)) {
      CkAbort("Error: Offset " ID_PRINT_TYPE " outside of valid range [0,"
        ID_PRINT_TYPE")\n", offset, numObjects);
    }
#endif  // ENABLE_DEBUG
  }
}

Id Partitioner::getLocalLocationIndex(Id globalIndex, PartitionId PartitionId) const {
  return getLocalIndex(globalIndex, PartitionId, locationPartitionOffsets);
}

Id Partitioner::getGlobalLocationIndex(Id localIndex, PartitionId PartitionId) const {
  return getGlobalIndex(localIndex, PartitionId, locationPartitionOffsets);
}

CacheOffset Partitioner::getPersonCacheIndex(Id globalIndex) const {
  return getLocalIndex(globalIndex, 0, locationPartitionOffsets);
}

PartitionId Partitioner::getLocationPartitionIndex(Id globalIndex) const {
  return getPartition(globalIndex, locationPartitionOffsets);
}

Id Partitioner::getLocationPartitionSize(PartitionId partitionIndex) const {
  return getPartitionSize(partitionIndex, numLocations, locationPartitionOffsets);
}

PartitionId Partitioner::getNumLocationPartitions() {
    return locationPartitionOffsets.size();
}

Id Partitioner::getLocalPersonIndex(Id globalIndex, PartitionId partitionIndex) const {
  return getLocalIndex(globalIndex, partitionIndex, personPartitionOffsets);
}

Id Partitioner::getGlobalPersonIndex(Id localIndex, PartitionId partitionIndex) const {
  return getGlobalIndex(localIndex, partitionIndex, personPartitionOffsets);
}

CacheOffset Partitioner::getLocationCacheIndex(Id globalIndex) const {
  return getLocalIndex(globalIndex, 0, personPartitionOffsets);
}

PartitionId Partitioner::getPersonPartitionIndex(Id globalIndex) const {
  return getPartition(globalIndex, personPartitionOffsets);
}

Id Partitioner::getPersonPartitionSize(PartitionId partitionIndex) const {
  return getPartitionSize(partitionIndex, numPeople, personPartitionOffsets);
}

PartitionId Partitioner::getNumPersonPartitions() {
    return personPartitionOffsets.size();
}

/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "Defs.h"
#include "charm++.h"

#include <cmath>
#include <algorithm>

/**
 * Returns the number of elements that each chare will track at a minimum.
 *
 * Args:
 *    Id numElements: The total number of objects of this type.
 *    Id numPartitions: The total number of chares.
 *
 */
Id getNumElementsPerPartition(Id numElements, Id numPartitions) {
  return ceil(static_cast<float>(numElements) / numPartitions);
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
 * Notes: Will likely change as we Idroduce active load balancing.
 */
Id getPartitionIndex(Id globalIndex, Id numElements, Id numPartitions, Id offset) {
  Id partitionIndex = (globalIndex - offset)
    / getNumElementsPerPartition(numElements, numPartitions);
  if (partitionIndex >= numPartitions)
    return (numPartitions - 1);
  return partitionIndex;
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
Id getFirstIndex(Id partitionIndex, Id numElements, Id numPartitions, Id offset) {
  Id elementsPerPartition = getNumElementsPerPartition(numElements,
      numPartitions);
  return partitionIndex * elementsPerPartition + offset;
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
Id getNumLocalElements(Id numElements, Id numPartitions, Id partitionIndex) {
  Id elementsPerPartition = getNumElementsPerPartition(numElements,
      numPartitions);
  Id firstIndex = getFirstIndex(partitionIndex, numElements, numPartitions,
      0);
  if (firstIndex >= numElements) {
    return 0;
  }
  return std::min(elementsPerPartition, numElements - firstIndex);
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
Id getGlobalIndex(Id localIndex, Id partitionIndex, Id numElements,
    Id numPartitions, Id offset) {
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
 *      object
 *    Id numElements: The total number of objects of this type.
 *    Id numPartitions: The total number of chares.
 *    Id offset: The globalIndex number referring to the first object. Likely
 *      could be replaced with a better system.
 *
 */
Id getLocalIndex(Id globalIndex, Id partitionIndex, Id numElements,
    Id numPartitions, Id offset) {
  Id firstLocalIndex = getFirstIndex(partitionIndex, numElements,
      numPartitions, offset);
  return globalIndex - firstLocalIndex;
}

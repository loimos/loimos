/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "Defs.h"
#include "charm++.h"

#include <cmath>

/**
 * Returns the number of elements that each chare will track at a minimum.
 *
 * Args:
 *    int numElements: The total number of objects of this type.
 *    int numPartitions: The total number of chares.
 *
 */
int getNumElementsPerPartition(int numElements, int numPartitions) {
  return floor(static_cast<float>(numElements)/numPartitions);
}


/**
 * Returns the number of the objects that a particular chare tracks.
 *
 * Args:
 *    int numElements: The total number of objects of this type.
 *    int numPartitions: The total number of chares.
 *    int partitionIndex: Chare index
 *
 */
int getNumLocalElements(int numElements, int numPartitions, int partitionIndex){
  int elementsPerPartition = getNumElementsPerPartition(numElements,
      numPartitions);
  if (partitionIndex == (numPartitions - 1))
    return numElements - elementsPerPartition * (numPartitions - 1);
  return elementsPerPartition;
}

/**
 * Returns the number of the chare that tracks a particular element.
 *
 * Args:
 *    int globalIndex: The global unique object identifier.
 *    int numElements: The total number of objects of this type.
 *    int numPartitions: The total number of chares.
 *    int offset: The globalIndex number referring to the first object. Likely
 *      could be replaced with a better system.
 *
 * Notes: Will likely change as we introduce active load balancing.
 */
int getPartitionIndex(int globalIndex, int numElements, int numPartitions, int offset) {
  int partitionIndex = (globalIndex - offset)
    / getNumElementsPerPartition(numElements, numPartitions);
  if (partitionIndex >= numPartitions)
    return (numPartitions - 1);
  return partitionIndex;
}

/**
 * Returns the first global index of an object present on the specified chare
 *
 * Args:
 *    int partitionIndex: The index of the chare in question
 *    int numElements: The total number of objects of this type.
 *    int numPartitions: The total number of chares.
 *    int offset: The globalIndex number referring to the first object. Likely
 *      could be replaced with a better system.
 *
 */
int getFirstIndex(int partitionIndex, int numElements, int numPartitions, int offset) {
  int elementsPerPartition = getNumElementsPerPartition(numElements,
      numPartitions);
  return partitionIndex * elementsPerPartition + offset;
}

/**
 * Returns global indexed position of an object from its local (zero-indexed)
 * id on a chare.
 *
 * Args:
 *    int localIndex: The local (zero-indexed) unique object identifier.
 *    int partitionIndex: The index of the chare containing the specified
 *      object
 *    int numElements: The total number of objects of this type.
 *    int numPartitions: The total number of chares.
 *    int offset: The globalIndex number referring to the first object. Likely
 *      could be replaced with a better system.
 *
 */
int getGlobalIndex(int localIndex, int partitionIndex, int numElements,
    int numPartitions, int offset) {
  int firstLocalIndex = getFirstIndex(partitionIndex, numElements,
      numPartitions, offset);
  return firstLocalIndex + localIndex;
}

/**
 * Returns local (zero-indexed) position of an object on a chare from its
 * global id.
 *
 * Args:
 *    int globalIndex: The global unique object identifier.
 *    int partitionIndex: The index of the chare containing the specified
 *      object
 *    int numElements: The total number of objects of this type.
 *    int numPartitions: The total number of chares.
 *    int offset: The globalIndex number referring to the first object. Likely
 *      could be replaced with a better system.
 *
 */
int getLocalIndex(int globalIndex, int partitionIndex, int numElements,
    int numPartitions, int offset) {
  int firstLocalIndex = getFirstIndex(partitionIndex, numElements,
      numPartitions, offset);
  return globalIndex - firstLocalIndex;
}

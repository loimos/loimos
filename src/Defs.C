/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __DEFS_H__
#define __DEFS_H__

#include "Defs.h"
#include <cmath>

int getNumElementsPerPartition(int numElements, int numPartitions){
  return floor((float)numElements/(float)numPartitions);
}

int getNumLocalElements(int numElements, int numPartitions, int partitionIndex){
  int elementsPerPartition = getNumElementsPerPartition(numElements,numPartitions);
  if(partitionIndex == (numPartitions-1))
    return numElements - elementsPerPartition*(numPartitions-1);
  return elementsPerPartition;
}

int getPartitionIndex(int globalIndex, int numElements, int numPartitions){
  int partitionIndex = globalIndex/getNumElementsPerPartition(numElements,numPartitions);
  if(partitionIndex >= numPartitions)
    return (numPartitions-1);
  return partitionIndex;
}

int getLocalIndex(int globalIndex, int numElements, int numPartitions){
  int partitionIndex = getPartitionIndex(globalIndex,numElements,numPartitions);
  int elementsPerPartition = getNumElementsPerPartition(numElements,numPartitions);
  return globalIndex - partitionIndex*elementsPerPartition;
}

int getGlobalIndex(int localIndex, int partitionIndex, int numElements, int numPartitions){
  int elementsPerPartition = getNumElementsPerPartition(numElements,numPartitions);
  return partitionIndex*elementsPerPartition+localIndex;
}

#endif // __DEFS_H__

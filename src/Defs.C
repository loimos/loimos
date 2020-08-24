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

int getNumLocalElems(int numElements, int numPartitions, int partitionIndex){
  int elemsPerCont = getNumElementsPerPartition(numElements,numPartitions);
  if(partitionIndex == (numPartitions-1))
    return numElements - elemsPerCont*(numPartitions-1);
  return elemsPerCont;
}

int getContainerIndex(int globalIndex, int numElements, int numPartitions){
  int partitionIndex = globalIndex/getNumElementsPerPartition(numElements,numPartitions);
  if(partitionIndex >= numPartitions)
    return (numPartitions-1);
  return partitionIndex;
}

int getLocalIndex(int globalIndex, int numElements, int numPartitions){
  int partitionIndex = getContainerIndex(globalIndex,numElements,numPartitions);
  int elemsPerCont = getNumElementsPerPartition(numElements,numPartitions);
  return globalIndex - partitionIndex*elemsPerCont;
}

int getGlobalIndex(int localIndex, int partitionIndex, int numElements, int numPartitions){
  int elemsPerCont = getNumElementsPerPartition(numElements,numPartitions);
  return partitionIndex*elemsPerCont+localIndex;
}

#endif // __DEFS_H__

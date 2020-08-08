/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __DEFS_H__
#define __DEFS_H__

#include "Defs.h"
#include <cmath>

int getElemsPerCont(int numElements, int numContainers){
  return floor((float)numElements/(float)numContainers);
}

int getNumLocalElems(int numElements, int numContainers, int containerIndex){
  int elemsPerCont = getElemsPerCont(numElements,numContainers);
  if(containerIndex == (numContainers-1))
    return numElements - elemsPerCont*(numContainers-1);
  return elemsPerCont;
}

int getContainerIndex(int globalIndex, int numElements, int numContainers){
  int contIndex = globalIndex/getElemsPerCont(numElements,numContainers);
  if(contIndex >= numContainers)
    return (numContainers-1);
  return contIndex;
}

int getLocalIndex(int globalIndex, int numElements, int numContainers){
  int contIndex = getContainerIndex(globalIndex,numElements,numContainers);
  int elemsPerCont = getElemsPerCont(numElements,numContainers);
  return globalIndex - contIndex*elemsPerCont;
}

int getGlobalIndex(int localIndex, int contIndex, int numElements, int numContainers){
  int elemsPerCont = getElemsPerCont(numElements,numContainers);
  return contIndex*elemsPerCont+localIndex;
}

#endif // __DEFS_H__

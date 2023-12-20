/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include <random>

#include "DataInterface.h"
#include "AttributeTable.h"
#include "../Types.h"

DataInterface::DataInterface(const AttributeTable &attributes, int numInterventions) {
  if (0 != numInterventions) {
    willComplyWithIntervention.resize(numInterventions);
  }

  // Treat all attributes same, no need to make distinction
  int tableSize = attributes.size();
  if (tableSize != 0) {
    data.resize(tableSize);
    for (int i = 0; i < tableSize; i++) {
      data[i] = attributes.getDefaultValue(i);
    }
  }
}

void DataInterface::setUniqueId(Id idx) {
  uniqueId = idx;
}

Id DataInterface::getUniqueId() const {
  return uniqueId;
}

union Data DataInterface::getValue(int idx) const {
  return data[idx];
}

std::vector<union Data> &DataInterface::getData() {
  return data;
}

void DataInterface::setSeed(int seed) {
  generator.seed(seed + uniqueId);
}

std::default_random_engine * DataInterface::getGenerator() {
  return &generator;
}

void DataInterface::toggleCompliance(int interventionIndex, bool value) {
  willComplyWithIntervention[interventionIndex] = value;
}

bool DataInterface::willComply(int interventionIndex) {
  return willComplyWithIntervention[interventionIndex];
}

/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "DataInterface.h"

void DataInterface::setUniqueId(int idx) {
  this->uniqueId = idx;
}

int DataInterface::getUniqueId() const {
  return this->uniqueId;
}

union Data DataInterface::getValue(int idx) const {
  return data[idx];
}

std::vector<union Data> &DataInterface::getData() {
  return this->data;
}

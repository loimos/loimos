/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef READERS_DATAINTERFACE_H_
#define READERS_DATAINTERFACE_H_

#include <vector>
#include <string>
#include <google/protobuf/repeated_field.h>

#include "charm++.h"
#include "pup_stl.h"
#include "../protobuf/data.pb.h"

namespace DataTypes {
  enum DataType {int_b10, uint_32, string, double_b10, category, boolean};
}

union Data {
  int int_b10;
  bool boolean;
  uint32_t uint_32;
  double double_b10;
  uint16_t category;
  std::string *str;
};
PUPbytes(union Data);

class DataInterface {
 protected:
  // Unique global identifier
  int uniqueId;
  // Various dynamic attributes
  std::vector<union Data> data;
 public:
  DataInterface() {}
  ~DataInterface() {}
  void setUniqueId(int idx);
  int getUniqueId() const;
  union Data getValue(int idx) const;
  std::vector<union Data> &getData();
};
#endif  // READERS_DATAINTERFACE_H_

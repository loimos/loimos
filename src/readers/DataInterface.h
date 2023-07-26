/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef READERS_DATAINTERFACE_H_
#define READERS_DATAINTERFACE_H_

#include <vector>
#include <string>
#include <functional>
#include <google/protobuf/repeated_field.h>

#include "charm++.h"
#include "pup_stl.h"
#include "../Message.h"
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

using VisitTest = std::function<bool(const VisitMessage &)>;

class DataInterface {
 protected:
  // Unique global identifier
  int uniqueId;
  // Various dynamic attributes
  std::vector<union Data> data;
 public:
  DataInterface() = default;
  virtual ~DataInterface() = default;
  void setUniqueId(int idx);
  int getUniqueId() const;
  union Data getValue(int idx) const;
  std::vector<union Data> &getData();
  virtual void filterVisits(const void *cause, VisitTest keepVisit) = 0;
  virtual void restoreVisits(const void *cause) = 0;
};
#endif  // READERS_DATAINTERFACE_H_

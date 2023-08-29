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
#include "Data.h"
#include "AttributeTable.h"
#include "../Types.h"
#include "../Message.h"
#include "../protobuf/data.pb.h"

class DataInterface {
 protected:
  // Unique global identifier
  Id uniqueId;

  // Various dynamic attributes
  std::vector<union Data> data;

  // Indicates whether or not this entity will comply with a given intervention
  std::vector<bool> willComplyWithIntervention;
 public:
  DataInterface() = default;
  DataInterface(const AttributeTable &attributes, int numInterventions);
  virtual ~DataInterface() = default;
  void setUniqueId(Id idx);
  Id getUniqueId() const;
  union Data getValue(int idx) const;
  std::vector<union Data> &getData();
  void toggleCompliance(int interventionIndex, bool value);
  bool willComply(int interventionIndex);
  virtual void filterVisits(const void *cause, VisitTest keepVisit) = 0;
  virtual void restoreVisits(const void *cause) = 0;
};
#endif  // READERS_DATAINTERFACE_H_

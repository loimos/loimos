/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef READERS_DATAINTERFACE_H_
#define READERS_DATAINTERFACE_H_

#include <vector>
#include <string>
#include <random>
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
  std::default_random_engine generator;

  // Various dynamic attributes
  std::vector<union Data> data;

  // Indicates whether or not this entity will comply with a given intervention,
  // whether or not said intervention is active, etc
  std::vector<InterventionStatus> interventionStatuses;

 public:
  DataInterface() = default;
  DataInterface(const AttributeTable &attributes, int numInterventions);
  virtual ~DataInterface() = default;
  void setUniqueId(Id idx);
  Id getUniqueId() const;
  union Data getValue(int idx) const;
  std::vector<union Data> &getData();
  void setSeed(int seed);
  std::default_random_engine * getGenerator();
  void toggleCompliance(InterventionId interventionIndex, bool value);
  inline int interventionIdToIndex(InterventionId id) const;
  bool willComply(InterventionId id) const;
  void toggleActivity(InterventionId id, bool value);
  bool isActive(InterventionId id) const;
  virtual void filterVisits(InterventionId interventionId, VisitTest keepVisit) = 0;
  virtual void restoreVisits(InterventionId interventionId, VisitTest restoreVisit) = 0;
};
#endif  // READERS_DATAINTERFACE_H_

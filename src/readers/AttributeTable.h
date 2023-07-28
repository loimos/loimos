 /* Copyright 2020-2023 The Loimos Project Developers.
  * See the top-level LICENSE file for details.
  *
  * SPDX-License-Identifier: MIT
  */

#ifndef INTERVENTION_MODEL_ATTRIBUTETABLE_H_
#define INTERVENTION_MODEL_ATTRIBUTETABLE_H_

#include "Data.h"
#include "../protobuf/data.pb.h"

#include <string>
#include <vector>

using AttributeList = google::protobuf::RepeatedPtrField<
  loimos::proto::DataField>;

struct Attribute {
  union Data defaultValue;
  DataTypes::DataType dataType;
  std::string name;

  Attribute() {}
  Attribute(union Data defaultValue_, DataTypes::DataType dataType_,
      std::string name_) : defaultValue(defaultValue_), dataType(dataType_),
      name(name_) {}
};

class AttributeTable {
 public:
  std::vector<Attribute> list;

  AttributeTable() {}
  explicit AttributeTable(int size);
  Attribute getAttribute(int i);
  union Data getDefaultValue(int i) const;
  double getDefaultValueAsDouble(int i) const;
  std::string getName(int i) const;
  DataTypes::DataType getDataType(int i);
  int getAttributeIndex(std::string name) const;
  int size() const;
  void updateIndex(int i, int newIndex);
  void resize(int size);
  void readAttributes(const AttributeList &dataFormat);
};

#endif  // INTERVENTION_MODEL_ATTRIBUTETABLE_H_

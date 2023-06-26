 /* Copyright 2020-2023 The Loimos Project Developers.
  * See the top-level LICENSE file for details.
  *
  * SPDX-License-Identifier: MIT
  */

#ifndef ATTRIBUTETABLE_H_
#define ATTRIBUTETABLE_H_

#include "../readers/DataInterface.h"
#include "../readers/DataReader.h"

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
  AttributeTable(int size, bool isPersonTable);
  explicit AttributeTable(bool isPersonTable);
  Attribute getAttribute(int i);
  union Data getDefaultValue(int i) const;
  std::string getName(int i) const;
  DataTypes::DataType getDataType(int i);
  bool getTableType();
  bool isPersonTable;
  int getAttribute(std::string name) const;
  int size() const;
  void updateIndex(int i, int newIndex);
  void resize(int size);
  void readAttributes(const AttributeList &dataFormat);
};

#endif  // ATTRIBUTETABLE_H_

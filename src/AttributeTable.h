 /* Copyright 2020-2023 The Loimos Project Developers.
  * See the top-level LICENSE file for details.
  *
  * SPDX-License-Identifier: MIT
  */

#ifndef ATTRIBUTETABLE_H_
#define ATTRIBUTETABLE_H_

#include "readers/DataInterface.h"
#include "readers/DataReader.h"

#include <string>
#include <vector>

struct Attribute {
  union Data defaultValue;
  DataType dataType;
  std::string name;
  int index;
};

class AttributeTable {
 public:
  std::vector<Attribute> list;
  AttributeTable(int size, bool isPersonTable);
  explicit AttributeTable(bool isPersonTable);
  Attribute getAttribute(int i);
  union Data getDefaultValue(int i) const;
  std::string getName(int i) const;
  DataType getDataType(int i);
  bool getTableType();
  bool isPersonTable;
  void populateTable(std::string fname);
  int getAttribute(std::string name) const;
  int size() const;
  void updateIndex(int i, int newIndex);
  void resize(int size);
  void readData(loimos::proto::CSVDefinition *dataFormat);
};

Attribute createAttribute(union Data val, DataType dtype,
    std::string name, int location);

#endif  // ATTRIBUTETABLE_H_

 /* Copyright 2020-2023 The Loimos Project Developers.
  * See the top-level LICENSE file for details.
  *
  * SPDX-License-Identifier: MIT
  */

#include "loimos.decl.h"
#include "AttributeTable.h"
#include "readers/data.pb.h"
#include "readers/DataReader.h"
#include "Person.h"
#include "Location.h"

#include <string>
#include <vector>

AttributeTable::AttributeTable(int size, bool isPersonTable) {
  if (size != 0) {
    this->list.reserve(size);
  }
  this->isPersonTable = isPersonTable;
}

AttributeTable::AttributeTable(bool isPersonTable) {
  this->isPersonTable = isPersonTable;
}
Attribute AttributeTable::getAttribute(int i) {
  return list[i];
}
union Data AttributeTable::getDefaultValue(int i) const {
  return list[i].defaultValue;
}
std::string AttributeTable::getName(int i) const {
  return list[i].name;
}
DataTypes::DataType AttributeTable::getDataType(int i) {
  return list[i].dataType;
}
bool AttributeTable::getTableType() {
  return this->isPersonTable;
}
int AttributeTable::size() const {
  return this->list.size();
}
void AttributeTable::resize(int size) {
  this->list.resize(size);
}

int AttributeTable::getAttribute(std::string name) const {
  for (int i = 0; i < this->size(); i++) {
    if (this->getName(i).c_str() == name) {
      return i;
    }
  }
  return -1;
}

void AttributeTable::populateTable(std::string fname) {
  std::ifstream infile(fname);
  std::string value;
  std::string dataType;
  std::string attributeType;
  std::string name;
  DataTypes::DataType dataEnum = DataTypes::int_b10;
  while (infile >> dataType >> name >> value >> attributeType) {
    // CkPrintf("Heads: %s, %s, %s, %s, %d\n", value.c_str(),
    //     dataType.c_str(), attributeType, name, test);
    if (dataType != "type" && ((attributeType == "p" && this->isPersonTable)
        || (attributeType == "l" && !this->isPersonTable))) {
      Data d;
      if (dataType == "double") {
        dataEnum = DataTypes::probability;
        d.probability = {std::stod(value)};
      } else if (dataType == "int" || dataType == "uint16_t") {
        dataEnum = DataTypes::int_b10;
        d.int_b10 = {std::stoi(value)};
      } else if (dataType == "bool") {
        dataEnum = DataTypes::boolean;
        d.boolean = {value == "1" || value == "t"};
      } else if (dataType == "uint32_t") {
        dataEnum = DataTypes::uint_32;
        d.uint_32 = {std::stoul(value)};
      }

      // CkPrintf("Att: %s\n", dataType);
      list.emplace_back(d, dataEnum, name);
    }
  }
}

void AttributeTable::readData(loimos::proto::CSVDefinition *dataFormat) {
  // this->reserve(DataReader<Person>::getNonZeroAttributes(dataFormat));
  for (int c = 0; c < dataFormat->fields_size(); c++) {
    loimos::proto::DataField const &field = dataFormat->fields(c);
    if (!field.has_ignore() && !field.has_unique_id()
        && !field.has_foreign_id()) {
      DataTypes::DataType dataEnum;
      Data d;
      if (field.has_b10int()) {
        d.int_b10 = 1;
        dataEnum = DataTypes::int_b10;
      } else if (field.has_label()) {
        d.str = new std::string("default");
        dataEnum = DataTypes::string;
      } else if (field.has_bool_()) {
        d.boolean = false;
        dataEnum = DataTypes::boolean;
      }

      list.emplace_back(d, dataEnum, field.field_name());
    }
  }
}


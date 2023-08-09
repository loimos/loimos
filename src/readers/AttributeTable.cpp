 /* Copyright 2020-2023 The Loimos Project Developers.
  * See the top-level LICENSE file for details.
  *
  * SPDX-License-Identifier: MIT
  */

#include "AttributeTable.h"
#include "DataReader.h"
#include "Data.h"
#include "../loimos.decl.h"
#include "../protobuf/data.pb.h"
#include "../Person.h"
#include "../Location.h"

#include <string>
#include <vector>

AttributeTable::AttributeTable(int size) {
  if (size != 0) {
    this->list.reserve(size);
  }
}

Attribute AttributeTable::getAttribute(int i) {
  return list[i];
}

union Data AttributeTable::getDefaultValue(int i) const {
  return list[i].defaultValue;
}

double AttributeTable::getDefaultValueAsDouble(int i) const {
  const union Data &defaultValue = list[i].defaultValue;
  switch (list[i].dataType) {
    case DataTypes::int_b10:
      return static_cast<double>(defaultValue.int_b10);

    case DataTypes::uint_32:
      return static_cast<double>(defaultValue.uint_32);

    case DataTypes::double_b10:
      return static_cast<double>(defaultValue.double_b10);

    case DataTypes::category:
      return static_cast<double>(defaultValue.category);

    case DataTypes::boolean:
      return static_cast<double>(defaultValue.boolean);
  }
}
std::string AttributeTable::getName(int i) const {
  return list[i].name;
}
DataTypes::DataType AttributeTable::getDataType(int i) {
  return list[i].dataType;
}
int AttributeTable::size() const {
  return this->list.size();
}
void AttributeTable::resize(int size) {
  this->list.resize(size);
}

int AttributeTable::getAttributeIndex(std::string name) const {
  for (int i = 0; i < this->size(); i++) {
    if (this->getName(i).c_str() == name) {
      return i;
    }
  }
  return -1;
}

void AttributeTable::readAttributes(const AttributeList &attributes) {
  for (int i = 0; i < attributes.size(); i++) {
    loimos::proto::DataField const &field = attributes[i];
    if (!field.has_ignore() && !field.has_unique_id()) {
      DataTypes::DataType type;
      Data defaultValue;
      if (field.has_b10int() || field.has_int32() || field.has_foreign_id()) {
        type = DataTypes::int_b10;
        if (field.default_value_case()
            == loimos::proto::DataField::DefaultValueCase::kDefaultInt) {
          defaultValue.int_b10 = field.default_int();
        } else {
          defaultValue.int_b10 = 0;
        }

      } else if (field.has_b10double() || field.has_double_()) {
        type = DataTypes::double_b10;
        if (field.default_value_case()
            == loimos::proto::DataField::DefaultValueCase::kDefaultDouble) {
          defaultValue.double_b10 = field.default_double();
        } else {
          defaultValue.double_b10 = 0.0;
        }

      } else if (field.has_label()) {
        type = DataTypes::string;
        if (field.default_value_case()
            == loimos::proto::DataField::DefaultValueCase::kDefaultString) {
          defaultValue.str = new std::string(field.default_string());
        } else {
          defaultValue.str = new std::string("");
        }

      } else if (field.has_bool_()) {
        type = DataTypes::boolean;
        defaultValue.boolean = false;
        if (field.default_value_case()
            == loimos::proto::DataField::DefaultValueCase::kDefaultBool) {
          defaultValue.boolean = field.default_bool();
        } else {
          defaultValue.boolean = false;
        }

      } else {
        CkAbort("Error: attribute \"%s\" has invalid type\n",
            field.field_name().c_str());
      }

      list.emplace_back(defaultValue, type, field.field_name());
    }
  }
}


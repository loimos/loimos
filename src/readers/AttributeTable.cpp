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
#include "../Types.h"
#include "../Defs.h"
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
    case DataTypes::int32_:
      return static_cast<double>(defaultValue.int32_val);

    case DataTypes::int64_:
      return static_cast<double>(defaultValue.int64_val);

    case DataTypes::uint32_:
      return static_cast<double>(defaultValue.uint32_val);

    case DataTypes::uint64_:
      return static_cast<double>(defaultValue.uint64_val);

    case DataTypes::double_:
      return static_cast<double>(defaultValue.double_val);

    case DataTypes::category_:
      return static_cast<double>(defaultValue.category_val);

    case DataTypes::bool_:
      return static_cast<double>(defaultValue.bool_val);
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

#define SET_DEFAULT(DEFAULT_VAR, TYPE_VAR, FIELD_VAR, TYPE, TYPE_CAP) do {\
  TYPE_VAR = DataTypes::CONCAT(TYPE, _);\
  if (field.default_value_case()\
    == loimos::proto::DataField::DefaultValueCase::CONCAT(kDefault, TYPE_CAP)) {\
      DEFAULT_VAR.CONCAT(TYPE, _val) = FIELD_VAR.CONCAT(default_, TYPE)();\
    } else {\
      DEFAULT_VAR.CONCAT(TYPE, _val) = 0;\
    }\
} while (0);


void AttributeTable::readAttributes(const AttributeList &attributes) {
  for (int i = 0; i < attributes.size(); i++) {
    loimos::proto::DataField const &field = attributes[i];
    if (!field.has_ignore() && !field.has_unique_id()) {
      DataTypes::DataType type;
      Data defaultValue;

      if (field.has_unique_id() || field.has_foreign_id()) {
        SET_DEFAULT(defaultValue, type, field, ID_PROTOBUF_TYPE,
          ID_PROTOBUF_TYPE_CAP);
      } else if (field.has_start_time() || field.has_duration()) {
        SET_DEFAULT(defaultValue, type, field, TIME_PROTOBUF_TYPE,
          TIME_PROTOBUF_TYPE_CAP);
      } else if (field.has_bool_()) {
        SET_DEFAULT(defaultValue, type, field, bool, Bool);
      } else if (field.has_int32()) {
        SET_DEFAULT(defaultValue, type, field, int32, Int32);
      } else if (field.has_int64()) {
        SET_DEFAULT(defaultValue, type, field, int64, Int64);
      } else if (field.has_uint32()) {
        SET_DEFAULT(defaultValue, type, field, uint32, Uint32);
      } else if (field.has_uint64()) {
        SET_DEFAULT(defaultValue, type, field, uint64, Uint64);
      } else if (field.has_double_()) {
        SET_DEFAULT(defaultValue, type, field, double, Double);
      } else if (field.has_string()) {
        type = DataTypes::string_;
        if (field.default_value_case()
            == loimos::proto::DataField::DefaultValueCase::kDefaultString) {
          defaultValue.string_val = new std::string(field.default_string());
        } else {
          defaultValue.string_val = new std::string("");
        }
      } else {
        CkAbort("Error: attribute \"%s\" has invalid type\n",
            field.field_name().c_str());
      }

      list.emplace_back(defaultValue, type, field.field_name());
    }
  }
}


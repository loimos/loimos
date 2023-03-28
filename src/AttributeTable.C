#include "loimos.decl.h"
#include "AttributeTable.h"
#include "readers/data.pb.h"
#include <string>
#include <vector>

AttributeTable::AttributeTable(int size, bool isPersonTable) {
    if (size != 0) {
        this->list.resize(size);
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
std::string AttributeTable::getName(int i) {
    return list[i].name;
}
std::string AttributeTable::getDataType(int i) {
    return list[i].dataType;
}
std::string AttributeTable::getAttributeType(int i) {
    return list[i].attributeType;
}
bool AttributeTable::getTableType() {
    return this->isPersonTable;
}
int AttributeTable::size() const {
    return this->list.size();
}
void AttributeTable::updateIndex(int i, int newIndex) {
    this->list[i].index = newIndex;
}
void AttributeTable::resize(int size) {
    this->list.resize(size);
}

//Convert attribute and index to actual value as type

Attribute createAttribute(union Data val, std::string dtype, std::string attype, std::string name, int index) {
    Attribute a;
    a.defaultValue = val;
    a.dataType = dtype;
    a.attributeType = attype;
    a.name = name;
    a.index = index;
    return a;
}

void AttributeTable::populateTable(std::string fname) {
    std::ifstream infile(fname);
    std::string value;
    std::string dataType;
    std::string attributeType;
    std::string name;
    while (infile >> dataType >> name >> value >> attributeType) {
      //CkPrintf("Heads: %s, %s, %s, %s, %d\n", value.c_str(),dataType.c_str(),attributeType,name, test);
      if (dataType != "type" && ((attributeType == "p" && this->isPersonTable) || (attributeType == "l" && !this->isPersonTable))) {
        Data d;
        if (dataType == "double") {
          d.probability = {std::stod(value)};
        } else if (dataType == "int" || dataType == "uint16_t") {
          d.int_b10 = {std::stoi(value)};
        } else if (dataType == "bool") {
          d.boolean = {value == "1" || value == "t"};
        } /*else if (dataType == "string") {
          d.*str = {value};
        }*/ else if (dataType == "uint32_t") {
          d.uint_32 = {std::stoul(value)};
        }
        //CkPrintf("Att: %s\n", dataType);
        this->list.emplace_back(createAttribute(d,dataType,attributeType,name,0));
      }  
    }
}



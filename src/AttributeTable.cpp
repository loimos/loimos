#include "loimos.decl.h"
#include "AttributeTable.h"
#include "readers/data.pb.h"
#include "readers/DataReader.h"
#include "Person.h"
#include "Location.h"
#include <string>
#include <vector>

using namespace DataTypes;

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
std::string AttributeTable::getName(int i) const {
    return list[i].name;
}
DataType AttributeTable::getDataType(int i) {
    return list[i].dataType;
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

Attribute createAttribute(union Data val, DataType dtype, std::string name, int index) {
    Attribute a;
    a.defaultValue = val;
    a.dataType = dtype;
    a.name = name;
    a.index = index;
    return a;
}

int AttributeTable::getAttribute(std::string name) const{
    for (int i = 0; i < this->size(); i++) {
        if (this->getName(i).c_str() == name) {
            return i;
        }
    }
    return -1;
}

/*void AttributeTable::pup(PUP::er &p) {
    p | list;
    p | isPersonTable;
}*/

void AttributeTable::populateTable(std::string fname) {
    std::ifstream infile(fname);
    std::string value;
    std::string dataType;
    std::string attributeType;
    std::string name;
    DataType dataEnum = int_b10;
    while (infile >> dataType >> name >> value >> attributeType) {
      //CkPrintf("Heads: %s, %s, %s, %s, %d\n", value.c_str(),dataType.c_str(),attributeType,name, test);
      if (dataType != "type" && ((attributeType == "p" && this->isPersonTable) || (attributeType == "l" && !this->isPersonTable))) {
        Data d;
        if (dataType == "double") {
          dataEnum = probability;
          d.probability = {std::stod(value)};
        } else if (dataType == "int" || dataType == "uint16_t") {
          dataEnum = int_b10;
          d.int_b10 = {std::stoi(value)};
        } else if (dataType == "bool") {
          dataEnum = boolean;
          d.boolean = {value == "1" || value == "t"};
        } /*else if (dataType == "string") {
          d.*str = {value};
        }*/ else if (dataType == "uint32_t") {
          dataEnum = uint_32;
          d.uint_32 = {std::stoul(value)};
        }
        //CkPrintf("Att: %s\n", dataType);
        this->list.emplace_back(createAttribute(d,dataEnum,name,0));
      }
    }
}

void AttributeTable::readData(loimos::proto::CSVDefinition *dataFormat) {
    this->resize(DataReader<Person>::getNonZeroAttributes(dataFormat));
    int attrInd = 0;
    bool isPid = true;
    for (int c = 0; c < dataFormat->fields_size(); c++) {
        if(!dataFormat->fields(c).has_ignore()) {
            if (!isPid) {
                loimos::proto::DataField const *field = &dataFormat->fields(c);
                DataType dataEnum = int_b10;
                Data d;
                std::string name = field->field_name();
                if (field->has_b10int() || field->has_foreign_id()) {
                    d.int_b10 = 1;
                    dataEnum = int_b10;
                } else if (field->has_label()) {
                    d.str = new std::string("default");
                    dataEnum = string;
                } else if (field->has_bool_()) {
                    d.boolean = true;
                }
                this->list[attrInd] = createAttribute(d,dataEnum,name,attrInd);

                attrInd++;
            } else {
                isPid = false;
            }
        }
    }
}




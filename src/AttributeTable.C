#include "loimos.decl.h"
#include "AttributeTable.h"
#include "readers/data.pb.h"
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
std::string AttributeTable::getName(int i) {
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

void AttributeTable::readData(std::ifstream *input, loimos::proto::CSVDefinition *dataFormat) {
            // TODO make this 2^16 and support longer lines through multiple reads.
    char buf[MAX_INPUT_lineLength];
        // Get next line.
    input->getline(buf, MAX_INPUT_lineLength);

    // Read over people data format.
    int attrIndex = 0;
    // Tracks how many non-ignored fields there have been.
    int numDataFields = 0;
    int leftCommaLocation = 0;


    int lineLength = input->gcount();
    for (int c = 0; c < lineLength; c++) {
        // Scan for the next attrbiutes - comma separted.
        if (buf[c] == CSV_DELIM || c + 1 == lineLength) {
            // Get next attribute type.
            CkPrintf("2ATTTABLESIZE %d attrindex %d\n", this->list.size(),attrIndex);
            //loimos::proto::Data_Field const *field = &dataFormat->field(attrIndex);
            uint16_t dataLen = c - leftCommaLocation;
            /*
            if (field->has_ignore() || dataLen == 0) {
                // Skip
            } else {
                // Process data.
                CkPrintf("4ATTTABLESIZE %d\n", this->list.size());
                char *start = buf + leftCommaLocation;
                if (c + 1 == lineLength) {
                    dataLen += 1;
                }
                Attribute a;
                DataType dataEnum = int_b10;
                Data d;
                std::string name = "testName";
                CkPrintf("5ATTTABLESIZE %d\n", this->list.size());
              // Parse byte stream to the correct representation.
                    CkPrintf("ATTRTABLEIND %d\n", numDataFields);
                  if (field->has_b10int() || field->has_foreignid()) {
                      // TODO parse this directly.
                      d.int_b10 = std::stoi(std::string(start, dataLen));
                      dataEnum = int_b10;
                  } else if (field->has_label()) {
                    d.str = new std::string(start, dataLen);
                      dataEnum = string;
                  } else if (field->has_bool_()) {
                      if (dataLen == 1) {
                          d.boolean = (start[0] == 't' || start[0] == '1');
                      } else {
                          d.boolean = false;
                      }
                  }
                  this->list[numDataFields] = createAttribute(d,dataEnum,name,numDataFields);
                  numDataFields++;
            }*/
        }
            leftCommaLocation = c + 1;
            attrIndex++;
    }
}




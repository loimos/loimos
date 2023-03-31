#ifndef __ATTRIBUTE_TABLE_H__
#define __ATTRIBUTE_TABLE_H__

#include "readers/DataInterface.h"
#include "readers/DataReader.h"
#include <string>
#include <vector>
using namespace DataTypes;
struct Attribute {
    union Data defaultValue;
    DataType dataType;
    std::string name;
    int index;
};
//Change dataTypes to enum in datareader.h


class AttributeTable
{
    
  public:
    std::vector<Attribute> list;
    AttributeTable( int size,  bool isPersonTable);
    AttributeTable( bool isPersonTable);
    Attribute getAttribute(int i);
    union Data getDefaultValue(int i) const;
    std::string getName(int i);
    DataType getDataType(int i);
    bool getTableType(); 
    bool isPersonTable;
    void populateTable(std::string fname);
    int size() const;
    void updateIndex(int i, int newIndex);
    void resize(int size);
    void readData(std::ifstream *input, loimos::proto::CSVDefinition *dataFormat);
};

Attribute createAttribute( union Data val,  DataType dtype, std::string name,  int location);



#endif

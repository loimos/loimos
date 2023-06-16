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

class AttributeTable
{
  public:
    std::vector<Attribute> list;
    AttributeTable( int size,  bool isPersonTable);
    AttributeTable( bool isPersonTable);
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
    //void pup(PUP::er &p);
};

Attribute createAttribute( union Data val,  DataType dtype, std::string name,  int location);

#endif

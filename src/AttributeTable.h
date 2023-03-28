#ifndef __ATTRIBUTE_TABLE_H__
#define __ATTRIBUTE_TABLE_H__

#include "readers/DataInterface.h"
#include "readers/DataReader.h"
#include <string>
#include <vector>

struct Attribute {
    union Data defaultValue;
    std::string dataType;
    std::string attributeType;
    //get rid of attribute type -- not important with table distinction now
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
    std::string getDataType(int i);
    std::string getAttributeType(int i);
    bool getTableType(); 
    bool isPersonTable;
    void populateTable(std::string fname);
    int size() const;
    void updateIndex(int i, int newIndex);
    void resize(int size);
};

Attribute createAttribute( union Data val,  std::string dtype,  std::string attype,  std::string name,  int location);



#endif

#ifndef __DATA_INTERFACE_H__
#define __DATA_INTERFACE_H__

class DataInterface {
    public:
        DataInterface(){};
        virtual ~DataInterface() {};
        virtual void setUniqueId(int idx) = 0;
        virtual union Data *getDataField() = 0;

};
#endif
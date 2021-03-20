/* Copyright 2021 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __DATA_INTERFACE_H__
#define __DATA_INTERFACE_H__

#include <vector>
// #include "pup_stl.h"

class DataInterface {
    public:
        DataInterface(){};
        virtual ~DataInterface() {};
        virtual void setUniqueId(int idx) = 0;
        virtual std::vector<union Data> getDataField() = 0;
};

namespace DataTypes {
    enum DataType { int_b10, uint_32, string, category }; 
}

union Data {
    int int_b10;
    bool boolean;
    uint32_t uint_32;
    uint16_t category; 

    void pup(PUP::er &p) {
        p|int_b10;
    }
};

#endif

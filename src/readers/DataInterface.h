/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __DATA_INTERFACE_H__
#define __DATA_INTERFACE_H__

#include "charm++.h"

#include <vector>
#include <string>

namespace DataTypes {
    enum DataType { int_b10, uint_32, string, probability, category };
}

union Data {
    int int_b10;
    bool boolean;
    uint32_t uint_32;
    double probability;
    uint16_t category;
    std::string *str;
};
PUPbytes(union Data);

class DataInterface {
    protected:
        // Unique global identifier
        int uniqueId;
        // Various dynamic attributes
        std::vector<union Data> data;
    public:
        DataInterface() {};
        ~DataInterface() {};
        void setUniqueId(int idx);
        int getUniqueId() const;
        union Data getValue(int idx) const;
        std::vector<union Data> &getData();
};
#endif //__DATA_INTERFACE_H__

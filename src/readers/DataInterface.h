/* Copyright 2021 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __DATA_INTERFACE_H__
#define __DATA_INTERFACE_H__

#include <vector>


namespace DataTypes {
    enum DataType { int_b10, uint_32, string, category }; 
}

union Data {
    int int_b10;
    bool boolean;
    uint32_t uint_32;
    uint16_t category; 
    double probability;
};

class DataInterface {
    private:
        std::vector<union Data> dataField;

    public:
        DataInterface(int numAttributes) {
            if (numAttributes != 0) {
                dataField.resize(numAttributes);
            }
        };

        // Defaults
        ~DataInterface() = default;
        DataInterface(const DataInterface&) = default;
        DataInterface(DataInterface&&) = default;
        // Default assignment operators.
        DataInterface& operator=(const DataInterface&) = default;
        DataInterface& operator=(DataInterface&&) = default;

        // Setters
        void setField(int index, union Data value) {
            dataField[index] = value;
        }
        const union Data& getField(int index) {
            return dataField.at(index);
        }
        std::vector<Data> &getDataField() {
            return dataField;
        }
};

#endif

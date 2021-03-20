/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "../loimos.decl.h"

#ifndef __DATA_INTERFACE_MESSAGE__
#define __DATA_INTERFACE_MESSAGE__

class DataInterfaceMessage : public CMessage_DataInterfaceMessage {
    public:
        int numDataAttributes;
        Data *dataAttributes;
        int uniqueId;

        DataInterfaceMessage(int attributes) {
            numDataAttributes = attributes;
            if (numDataAttributes != 0) {
                dataAttributes = new Data[numDataAttributes];
                assert(dataAttributes != NULL);
            }
        }
        
        void pup(PUP::er &p) {
            p|uniqueId;
            p|numDataAttributes;
            // if (p.isUnpacking() && numDataAttributes != 0) {
            if (numDataAttributes != 0) {
                PUParray(p, dataAttributes, numDataAttributes);
            }
            
        }
};

#endif // __DATA_INTERFACE_MESSAGE__
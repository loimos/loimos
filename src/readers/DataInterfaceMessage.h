/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "../loimos.decl.h"

class DataInterfaceMessage : CMessage_DataInterfaceMessage {
    public:
        DataInterfaceMessage() {}
        int numDataAttributes;
        void **dataAttributes;
        // union Data *dataAttributes;
        int uniqueId;
};
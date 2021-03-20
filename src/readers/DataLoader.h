/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __DATALOADER_H__
#define __DATALOADER_H__

#include "../loimos.decl.h"

#include <vector>
#include <stdio.h>
#include <string>
#include <fstream>

#include "data.pb.h"
#include "DataInterface.h"
#include "DataInterfaceMessage.h"

#define MAX_INPUT_lineLength (std::streamsize) 262144 // 2^18

/**
 * Defines a generic data reader for any child class of DataInterface.
 */ 
class DataLoader : public CBase_DataLoader {
    public:
        DataLoader();
        void BeginDataLoading();
        void readData(std::ifstream *input, loimos::proto::CSVDefinition *dataFormat, DataInterfaceMessage *msg)
        static int getNumberOfDataAttributes(loimos::proto::CSVDefinition *dataFormat);
        static std::tuple<int, int, int, int> parseActivityStream(std::ifstream *input, loimos::proto::CSVDefinition *dataFormat, std::vector<union Data> *attributes);
};

#endif // __DATALOADER_H__
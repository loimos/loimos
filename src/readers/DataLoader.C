/* Copyright 2020 The Loimos Project Developers.
* See the top-level LICENSE file for details.
*
* SPDX-License-Identifier: MIT
*/

#include "../loimos.decl.h"
#include "../Defs.h"

#include "DataLoader.h"
#include "DataInterfaceMessage.h"

DataLoader::DataLoader() {

}

DataLoader::BeginDataLoading() {
    // Determine attributes of this data loader.
    int numPeopleChares = numPeoplePartitions / LOADING_CHARES_PER_CHARE;
    bool personReader = thisIndex < numPeopleChares;
    int charesOfType = personReader ? numPeoplePartitions : numLocationPartitions;
    int relIndex = personReader ? thisIndex : personReader - numPeopleChares;
    int lowerChare = relIndex * LOADING_CHARES_PER_CHARE;
    int upperChare = min(charesOfType, (relIndex + 1) * LOADING_CHARES_PER_CHARE);
    
    // Open relevant files.
    std::string dataPath = scenarioPath
                        + (personReader ? "people.csv" : "locations.csv");
    std::ifstream dataStream(dataPath);
    std::string cachePath = scenarioPath 
                        + (personReader ? "_people.cache" : "_locations.cache");
    std::ifstream cacheStream(cachePath);
    if (!dataStream || !cacheStream) {
        CkAbort("Could not open person data input.");
    }

    // Create object to read into.
    int numAttributesPerObj = personReader ? 
        DataReader::getNumberOfDataAttributes(diseaseModel->personDef) :
        DataReader::getNumberOfDataAttributes(diseaseModel->locationDef); 

    // Read in data for each chare sequentially.
    for (int targetChare = lowerChare; targetChare < upperChare; upperChare++) {
        // Seek to correct position in cache.
        cacheStream.seekg(index * sizeof(uint32_t));
        uint32_t offset;
        cacheStream.read((char *) &offset, sizeof(uint32_t));
        dataStream.seekg(offset);

        // // Message freed by system.
        // DataInterfaceMessage *msg = new DataInterfaceMessage(numAttributesPerObj, 0);

        // if (personReader) {
        //     DataReader::readData(&dataPath, diseaseModel->personDef, msg);
        //     peopleArray[targetChare].ReceivePersonSetup(msg);
        // } else {
        //     DataReader::readData(&dataPath, diseaseModel->locationDef, msg);
        //     locationsArray[targetChare].ReceiveLocationSetup(msg);
        // }
    }

    // Cleanup.
    free(msg);
    dataStream.close();
    cacheSteam.close();
}

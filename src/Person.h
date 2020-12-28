
#ifndef __PERSON_H__
#define __PERSON_H__

#include "data/DataInterface.h"
#include "data/DataReader.h"

class Person : public DataInterface {
    public:
        // Unique global identifier for a person.
        int uniqueId;
        // Numeric disease state of the person.
        int state;
        int secondsLeftInState;
        // Byte offsets creating an index of this persons visits.
        std::vector<uint32_t> interactionsByDay;

        // Various attributes of the person.
        union Data *personData;

        // Methods
        Person(int numAttributes, int startingState, int timeLeftInState);
        ~Person();
        void setUniqueId(int idx);
        union Data *getDataField();
        void _print_information(loimos::proto::CSVDefinition *personDef);
};
#endif
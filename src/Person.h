/* Copyright 2021 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __PERSON_H__
#define __PERSON_H__

#include "Defs.h"
#include "Message.h"
#include "readers/DataInterface.h"
#include "readers/DataReader.h"

class Person : public DataInterface {
    public:
        // Unique global identifier for a person.
        int uniqueId;
        // Numeric disease state of the person.
        int state;
        int next_state;
        int secondsLeftInState;
        bool willComply;
        bool isIsolating;

        // If this is a susceptible person, this is a list of all of their
        // interactions with infectious people in the past day
        std::vector<Interaction> interactions;

        // Integer byte offsets in visits file by day.
        // For example, fseek(visitOffsetByDay[2]) would seek to the start
        // of this person's visits on day 3.
        std::vector<uint32_t> visitOffsetByDay;

        // Holds visit messages for each day
        std::vector<std::vector<VisitMessage> > visitsByDay;
        
        // Various dynamic attributes of the person
        std::vector<union Data> personData;

        // Constructors and assignment operators
        Person() = default;
        Person(int numAttributes, int startingState, int timeLeftInState);
        Person(const Person&) = default;
        Person(Person&&) = default;
        Person& operator=(const Person&) = default;
        Person& operator=(Person&&) = default;
        ~Person() = default;

        // Lets charm++ migrate objects
        void pup(PUP::er &p);

        // Disease model functions.
        void EndOfDayStateUpdate(DiseaseModel *diseaseModel,
                                   std::default_random_engine *generator);

        // Override DataInterfect abstract methods
        void setUniqueId(int idx);
        std::vector<union Data> &getDataField();

        // Debugging.
        void _print_information(loimos::proto::CSVDefinition *personDef);
};
#endif

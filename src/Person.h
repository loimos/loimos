/* Copyright 2021 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __PERSON_H__
#define __PERSON_H__

#include "Defs.h"

class Person : public DataInterface {
    public:
        // Unique global identifier for a person.
        int uniqueId;
        // Numeric disease state of the person.
        int state;
        int next_state;
        int secondsLeftInState;

        // If this is a susceptible person, this is a list of all of their
        // interactions with infectious people in the past day
        std::vector<Interaction> interactions;

        // Integer byte offsets in interaction file by data for a persons visit.
        // for example fseek(interactionsByDay[2]) would seek to the start
        // of this persons interactions on day 3.
        std::vector<uint32_t> interactionsByDay;
        
        // Various dynamic attributes of the person
        std::vector<union Data> personData;

        // Constructors and assignment operators
        Person(int uniqueId, int numAttributes, int startingState, int timeLeftInState);
        Person(const Person&) = default;
        Person(Person&&) = default;
        Person& operator=(const Person&) = default;
        Person& operator=(Person&&) = default;
        ~Person() = default;
};
#endif

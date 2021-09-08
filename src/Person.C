/* Copyright 2021 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"

#include "Person.h"
#include "readers/data.pb.h"

/**
 * Defines attributes of a single person.
 */ 

Person::Person(int uniqueId, int numAttributes, int startingState, int timeLeftInState) 
    : DataInterface (numAttributes)    
{
    if (numAttributes != 0) {
        this->personData.resize(numAttributes);
    }
    this->uniqueId = uniqueId;
    this->state = startingState;
    this->secondsLeftInState = timeLeftInState;
    this->interactionsByDay = std::vector<uint32_t>();
    this->next_state = -1;
}

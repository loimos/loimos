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

Person::Person(int numAttributes, int startingState, int timeLeftInState) {
    if (numAttributes != 0) {
        this->personData.resize(numAttributes);
    }
    this->state = startingState;
    this->secondsLeftInState = timeLeftInState;
    this->interactionsByDay = std::vector<uint32_t>();
}

void Person::setUniqueId(int idx) {
    this->uniqueId = idx;
}

std::vector<union Data> Person::getDataField() {
    return this->personData;
}

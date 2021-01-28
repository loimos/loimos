/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"

#include "Person.h"
#include "data/data.pb.h"

/**
 * Defines attributes of a single person.
 */ 

Person::Person(int numAttributes, int startingState, int timeLeftInState) {
    this->personData = (union Data *) malloc(numAttributes * sizeof(union Data));
    this->state = startingState;
    this->secondsLeftInState = timeLeftInState;
    this->interactionsByDay = std::vector<uint32_t>();
}

Person::~Person() {
    free(this->personData);
}

void Person::setUniqueId(int idx) {
    this->uniqueId = idx;
}

union Data *Person::getDataField() {
    return this->personData;
}

void Person::_print_information(loimos::proto::CSVDefinition *personDef) {
    printf("My ID is %d.\n", this->uniqueId);
    int attr = 0;
    for (int c = 0; c < personDef->field_size(); c++) {
        loimos::proto::Data_Field const *field = &personDef->field(c);
        if (field->has_ignore()) {
            // Skip
        } else {
            printf("-- %s is ", field->field_name().c_str());
            if (field->has_uniqueid() || field->has_b10int() || field->has_foreignid()) {
                printf("%d\n", this->personData[attr].int_b10);
            } else if (field->has_label()) {
                printf("%s\n", this->personData[attr].str.c_str());
            } else if (field->has_bool_()) {
                printf("%s\n", this->personData[attr].boolean ? "True" : "False");
            }
            attr++;
        }
    }
}
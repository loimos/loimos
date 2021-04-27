/* Copyright 2021 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"

#include "Person.h"
#include "readers/data.pb.h"
#include "DiseaseModel.h"
/**
 * Defines attributes of a single person.
 */ 

Person::Person(int numAttributes, int startingState, int timeLeftInState) {
    if (numAttributes != 0) {
        this->personData.resize(numAttributes);
    }
    this->state = startingState;
    this->secondsLeftInState = timeLeftInState;
    this->visitOffsetByDay = std::vector<uint32_t>();
    this->next_state = -1;
}

void Person::setUniqueId(int idx) {
    this->uniqueId = idx;
}

std::vector<union Data> &Person::getDataField() {
    return this->personData;
}

void Person::MakeDiseaseTransition(DiseaseModel *diseaseModel, std::default_random_engine *generator) {
  // Transition to next state or mark the passage of time
  secondsLeftInState -= DAY_LENGTH;
  if (secondsLeftInState <= 0) {
    // If they have already been infected
    if (next_state != -1) {
      state = next_state;
      std::tie(next_state, secondsLeftInState) = 
        diseaseModel->transitionFromState(state, generator);
    } else {
      // Get which exposed state they should transition to.
      std::tie(state, std::ignore) = 
        diseaseModel->transitionFromState(state, generator);
      // See where they will transition next.
      std::tie(next_state, secondsLeftInState) =
        diseaseModel->transitionFromState(state, generator);
    }
  } else {
    secondsLeftInState = secondsLeftInState;
  }
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
                // printf("%s\n", this->personData[attr].str.c_str());
            } else if (field->has_bool_()) {
                printf("%s\n", this->personData[attr].boolean ? "True" : "False");
            }
            attr++;
        }
    }
}

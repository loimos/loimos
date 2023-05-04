/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"

#include "Person.h"
#include "DiseaseModel.h"
#include "Message.h"
#include "readers/data.pb.h"

#include <vector>

/**
 * Defines attributes of a single person.
 */

Person::Person(int numAttributes, int startingState, int timeLeftInState) {
  if (numAttributes != 0) {
    this->data.resize(numAttributes);
  }
  this->state = startingState;
  this->next_state = -1;
  this->isIsolating = false;
  this->willComply = false;
  this->secondsLeftInState = timeLeftInState;
  this->visitOffsetByDay = std::vector<uint64_t>();

  // Create an entry for each day we have data for
  this->visitsByDay.resize(numDaysWithRealData);
}

void Person::pup(PUP::er &p) {
  p | uniqueId;
  p | state;
  p | next_state;
  p | secondsLeftInState;
  p | interactions;
  p | visitOffsetByDay;
  p | visitsByDay;
  p | data;
  p | isIsolating;
}

void Person::EndOfDayStateUpdate(DiseaseModel *diseaseModel,
    std::default_random_engine *generator) {
  // Transition to next state or mark the passage of time
  secondsLeftInState -= DAY_LENGTH;
  if (secondsLeftInState <= 0) {
    // If they have already been infected
    if (next_state != -1) {
      state = next_state;
      std::tie(next_state, secondsLeftInState) =
        diseaseModel->transitionFromState(state, generator);

      // Check if person will begin isolating.
      if (willComply) {
        isIsolating = diseaseModel->shouldPersonIsolate(state);
      }

    } else {
      // Get which exposed state they should transition to.
      std::tie(state, std::ignore) =
        diseaseModel->transitionFromState(state, generator);
      // See where they will transition next.
      std::tie(next_state, secondsLeftInState) =
        diseaseModel->transitionFromState(state, generator);
    }
  }
}

void Person::_print_information(loimos::proto::CSVDefinition *personDef) {
  printf("My ID is %d.\n", this->uniqueId);
  int attr = 0;
  for (int c = 0; c < personDef->fields_size(); c++) {
    loimos::proto::DataField const *field = &personDef->fields(c);
    if (field->has_ignore()) {
      // Skip
    } else {
      printf("-- %s is ", field->field_name().c_str());
      if (field->has_unique_id() || field->has_b10int()
          || field->has_foreign_id()) {
        printf("%d\n", this->data[attr].int_b10);
      } else if (field->has_label()) {
        // printf("%s\n", this->data[attr].str.c_str());
      } else if (field->has_bool_()) {
        printf("%s\n", this->data[attr].boolean ? "True" : "False");
      }
      attr++;
    }
  }
}

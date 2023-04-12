/* Copyright 2021 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"

#include "Person.h"
#include "DiseaseModel.h"
#include "Message.h"
#include "readers/data.pb.h"

#include <random>

/**
 * Defines attributes of a single person.
 */

Person::Person(int numAttributes, int startingState, int timeLeftInState) {
    if (numAttributes != 0) {
        this->personData.resize(numAttributes);
    }
    this->state = startingState;
    this->nextState = -1;
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
    p | nextState;
    p | secondsLeftInState;
    p | interactions;
    p | visitOffsetByDay;
    p | visitsByDay;
    p | personData;
    p | isIsolating;
}

void Person::setUniqueId(int idx) {
    this->uniqueId = idx;
}

std::vector<union Data> &Person::getDataField() {
    return this->personData;
}

void Person::EndOfDayStateUpdate(DiseaseModel *diseaseModel,
    std::default_random_engine *generator) {
  // We use the max time to mark states which last indefintiely
  if (std::numeric_limits<Time>::max() == secondsLeftInState) {
    return;
  }
  secondsLeftInState -= DAY_LENGTH;

  int dwellTime = 0;
  int oldState = state;
  if (secondsLeftInState <= 0) {
    // If they have already been infected
    if (nextState != -1) {
      state = nextState;
      std::tie(nextState, dwellTime) =
        diseaseModel->transitionFromState(state, generator);

      // Check if person will begin isolating.
      if (willComply) {
        isIsolating = diseaseModel->shouldPersonIsolate(state);
      }

    } else {
      // Get which exposed state they should transition to.
      std::tie(state, std::ignore) =
        diseaseModel->transitionFromState(state, generator);
      // See where they will transition next
      std::tie(nextState, dwellTime) =
        diseaseModel->transitionFromState(state, generator);
    }

    // We use the max time to mark states which last indefintiely, so don't
    // mess that up
    if (std::numeric_limits<Time>::max() == dwellTime) {
      secondsLeftInState = dwellTime;
    } else {
      secondsLeftInState += dwellTime;
    }
    CkPrintf("  Person %d transitioned from %d to %d (%d/%ds left, next %d)\n",
        uniqueId, oldState, state, secondsLeftInState, dwellTime, nextState);
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

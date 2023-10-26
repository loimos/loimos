/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "Person.h"
#include "Message.h"
#include "protobuf/data.pb.h"
#include "readers/AttributeTable.h"

#include "charm++.h"
#include <vector>

/**
 * Defines attributes of a single person.
 */

Person::Person(const AttributeTable &attributes, int numInterventions,
    int startingState, int secondsLeftInState_, int numDays) :
    DataInterface(attributes, numInterventions),
    state(startingState), next_state(-1),
    secondsLeftInState(secondsLeftInState_) {
  // Create an entry for each day we have data for
  this->visitsByDay.resize(numDays);
}

void Person::filterVisits(const void *cause, VisitTest keepVisit) {
  for (std::vector<VisitMessage> &visits : visitsByDay) {
    for (int i = 0; i < visits.size(); ++i) {
      if (!keepVisit(visits[i])) {
        visits[i].deactivatedBy = cause;
      }
    }
  }
}

void Person::restoreVisits(const void *cause) {
  for (std::vector<VisitMessage> &visits : visitsByDay) {
    for (int i = 0; i < visits.size(); ++i) {
      if (cause == visits[i].deactivatedBy) {
        visits[i].deactivatedBy = NULL;
      }
    }
  }
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
}

void Person::_print_information(loimos::proto::CSVDefinition *personDef) {
  printf("My ID is "ID_PRINT_TYPE".\n", this->uniqueId);
  int attr = 0;
  for (int c = 0; c < personDef->fields_size(); c++) {
    loimos::proto::DataField const *field = &personDef->fields(c);
    if (field->has_ignore()) {
      // Skip
    } else {
      printf("-- %s is ", field->field_name().c_str());
      if (field->has_unique_id() || field->has_int32()
          || field->has_foreign_id()) {
        printf("%d\n", this->data[attr].int32_val);
      } else if (field->has_string()) {
        // printf("%s\n", this->data[attr].str.c_str());
      } else if (field->has_bool_()) {
        printf("%s\n", this->data[attr].bool_val ? "True" : "False");
      }
      attr++;
    }
  }
}

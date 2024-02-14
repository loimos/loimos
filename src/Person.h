/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PERSON_H_
#define PERSON_H_

#include "Defs.h"
#include "Message.h"
#include "readers/DataInterface.h"
#include "readers/AttributeTable.h"

#include "charm++.h"
#include <vector>

class Person : public DataInterface {
 public:
  // Numeric disease state of the person.
  DiseaseState state;
  DiseaseState next_state;
  Time secondsLeftInState;
  bool updated;

  // If this is a susceptible person, this is a list of all of their
  // interactions with infectious people in the past day
  std::vector<Interaction> interactions;

  // Integer byte offsets in visits file by day.
  // For example, fseek(visitOffsetByDay[2]) would seek to the start
  // of this person's visits on day 3.
  std::vector<CacheOffset> visitOffsetByDay;

  // Holds visit messages for each day
  std::vector<std::vector<VisitMessage> > visitsByDay;

  // Constructors and assignment operators
  Person() = default;
  Person(const AttributeTable &attributes, int numInterventions,
    DiseaseState startingState, Time timeLeftInState, int numDays);
  Person(const Person&) = default;
  Person(Person&&) = default;
  Person& operator=(const Person&) = default;
  Person& operator=(Person&&) = default;
  ~Person() = default;
  void filterVisits(const void *cause, VisitTest keepVisit) override;
  void restoreVisits(const void *cause) override;
  // Lets charm++ migrate objects
  void pup(PUP::er &p);  // NOLINT(runtime/references)
  // Debugging.
  void _print_information(loimos::proto::CSVDefinition *personDef);
};
#endif  // PERSON_H_

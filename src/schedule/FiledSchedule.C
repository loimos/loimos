/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "../loimos.decl.h"
#include "FiledSchedule.h"
#include "../Person.h"

#include <vector>
#include <iostream>

void FiledSchedule::setActivityData(std::ifstream *activityData) {
  this->activityData = activityData;
}

void FiledSchedule::sendVisitMessages(
  const std::vector<Person> &people,
  int peopleChareIndex
) {}

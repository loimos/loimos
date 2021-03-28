/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "../loimos.decl.h"
#include "Schedule.h"
#include "../Person.h"

#include <random>
#include <vector>
#include <iostream>

void Schedule::setSeed(const int seed) {
  this->seed = seed;
  generator.seed(seed);
}

void Schedule::setActivityData(std::ifstream *activityData) {
  this->activityData = activityData;
}

void Schedule::sendVisitMessages(const std::vector<Person> &people) {}

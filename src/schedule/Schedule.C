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

// These are just stubs so that we can actually make varibales of type Schedule
void Schedule::setSeed(const int seed) {}

void Schedule::setActivityData(std::ifstream *activityData) {}

void Schedule::sendVisitMessages(const std::vector<Person> &people) {}

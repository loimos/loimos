/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "../loimos.decl.h"
#include "SyntheticSchedule.h"
#include "../Person.h"

#include <random>
#include <vector>

void SyntheticSchedule::setSeed(const int seed) {
  this->seed = seed;
  generator.seed(seed);
}

void SyntheticSchedule::sendVisitMessages(const std::vector<Person> &people) {}

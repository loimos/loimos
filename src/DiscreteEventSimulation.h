/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef DISCRETEEVENTSIMULATION_H_
#define DISCRETEEVENTSIMULATION_H_

#include "Location.h"
#include "Scenario.h"

// Runs through all of the current events and return the indices of
// any people who have been infected
Counter processEvents(Location *loc, Scenario *scenario,
    std::ofstream *interactionsFile, int thisIndex);

#endif  // DISCRETEEVENTSIMULATION_H_
/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "../loimos.decl.h"
#include "../Location.h"
#include "../Event.h"
#include "../Defs.h"
#include "../readers/DataReader.h"
#include "ContactModel.h"
#include "MinMaxAlphaModel.h"

#include <vector>
#include <random>

// Constants used to determine each location's contact probability
const unsigned int MIN = 5;
const unsigned int MAX = 40;
const unsigned int ALPHA = 1000;

MinMaxAlphaModel::MinMaxAlphaModel() {
  contactProbabilityIndex = -1;
}

// Compute this location's contact probability and store it as an attribute
void MinMaxAlphaModel::computeLocationValues(Location& location) {
  std::vector<union Data> &data = location.getDataField();
  
  if (-1 == contactProbabilityIndex) {
    contactProbabilityIndex = (int) data.size();
  }

  // This is the probability (NOT propensity) of two people who are a location
  // at the same time coming into contact. MIN, MAX, and ALPHA are constant for
  // all locations, but the maximum number if simulateous visits (max_visits)
  // varries by location
  union Data contactProbability;
  double max_visits = (double) data[SIMULTANEOUS_MAX_VISITS_CSV_INDEX].uint_32;
  CkPrintf("max_simultaneous visits %f\r\n", max_visits);
  contactProbability.probability = fmin(
    1,
    (MIN + (MAX - MIN) * (1.0 - exp(-max_visits / ALPHA))) / (max_visits - 1)
  );

  data.push_back(contactProbability);
}

// Use this location's contact probability
bool MinMaxAlphaModel::madeContact(
  const Event& susceptibleEvent,
  const Event& infectiousEvent,
  Location& location
) {
  union Data contactProbability = 
    location.getDataField()[contactProbabilityIndex];
  return unitDistrib(*generator) < contactProbability.probability;
}

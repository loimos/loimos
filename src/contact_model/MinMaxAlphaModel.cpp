/* Copyright 2020-2023 The Loimos Project Developers.
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
void MinMaxAlphaModel::computeLocationValues(Location *location) {
  std::vector<union Data> &data = location->getData();

  if (-1 == contactProbabilityIndex) {
    contactProbabilityIndex = static_cast<int>(data.size());
  }

  // This is the probability (NOT propensity) of two people who are a location
  // at the same time coming into contact. MIN, MAX, and ALPHA are constant for
  // all locations, but the maximum number if simultaneous visits (max_visits)
  // varies by location
  union Data contactProbability;
  double max_visits =
    static_cast<double>(data[SIMULTANEOUS_MAX_VISITS_CSV_INDEX].uint_32);
  contactProbability.double_b10 = fmin(1,
    (MIN + (MAX - MIN) * (1.0 - exp(-max_visits / ALPHA))) / (max_visits - 1));

  data.push_back(contactProbability);
}

// Use this location's contact probability
bool MinMaxAlphaModel::madeContact(
  const Event &susceptibleEvent,
  const Event &infectiousEvent,
  const Location &location
) {
  union Data contactProbability =
    location.getValue(contactProbabilityIndex);
  return unitDistrib(*generator) < contactProbability.double_b10;
}

double MinMaxAlphaModel::getContactProbability(const Location &location) const {
  union Data contactProbability =
    location.getValue(contactProbabilityIndex);
  return contactProbability.probability;
}

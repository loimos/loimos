/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "../loimos.decl.h"
#include "../Location.h"
#include "../Event.h"
#include "../Defs.h"
#include "../Extern.h"
#include "../readers/DataReader.h"
#include "../readers/AttributeTable.h"
#include "ContactModel.h"
#include "MinMaxAlphaModel.h"

#include <vector>
#include <random>

// Constants used to determine each location's contact probability
const unsigned int MIN = 5;
const unsigned int MAX = 40;
const unsigned int ALPHA = 1000;

MinMaxAlphaModel::MinMaxAlphaModel(const AttributeTable &attrs) :
    ContactModel(attrs) {
  contactProbabilityIndex = -1;
  maxSimVisitsIndex = attrs.getAttributeIndex("max_simultaneous_visits");

  if (-1 == maxSimVisitsIndex) {
    CkAbort("Error: required attribute \"max_simultaneous_visits\" not present\n");
  } else {
#if ENABLE_DEBUG >= DEBUG_VERBOSE
    if (0 == CkMyNode()) {
      CkPrintf("  Max sim visit count to be stored at index %d\n",
          maxSimVisitsIdx);
    }
#endif
  }
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
    static_cast<double>(data[maxSimVisitsIndex].int32_val);
  contactProbability.double_val = fmin(1,
    (MIN + (MAX - MIN) * (1.0 - exp(-max_visits / ALPHA))) / (max_visits - 1));

  data.push_back(contactProbability);
}

// Use this location's contact probability
bool MinMaxAlphaModel::madeContact(
  const Event &susceptibleEvent,
  const Event &infectiousEvent,
  Location *location
) {
  union Data contactProbability =
    location->getValue(contactProbabilityIndex);
  return unitDistrib(*location->getGenerator()) < contactProbability.double_val;
}

double MinMaxAlphaModel::getContactProbability(const Location &location) const {
  union Data contactProbability =
    location.getValue(contactProbabilityIndex);
  return contactProbability.double_val;
}

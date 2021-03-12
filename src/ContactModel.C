/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "ContactModel.h"
#include "Location.h"
#include "Event.h"
#include "readers/DataReader.h"

#include <vector>
#include <random>

const unsigned int A = 1;
const unsigned int B = 2;
const unsigned int ALPHA = 1;

ContactModel::ContactModel() {
  unitDistrib = std::uniform_real_distribution<>(0.0, 1.0);
  contactProbabilityIndex = -1;
}

void ContactModel::setGenerator(std::default_random_engine *generator) {
  this->generator = generator;
}

void ContactModel::computeLocationValues(Location& location) {
  std::vector<union Data> &data = location.getDataField();
  
  if (-1 == contactProbabilityIndex) {
    contactProbabilityIndex = (int) data.size();
  }

  double max_visits = (double) data[LOCATIONS_MAX_VISITS_INDEX].uint_32;
  union Data contactProbability;
  contactProbability.probability = fmin(
    1,
    (A + (B - A) * (1.0 - exp(-max_visits / ALPHA))) / (max_visits - 1)
  );
  data.push_back(contactProbability);
}

bool ContactModel::madeContact(
  const Event& susceptibleEvent,
  const Event& infectiousEvent,
  Location& location
) {
  union Data contactProbability = 
    location.getDataField()[contactProbabilityIndex];
  return unitDistrib(*generator) < contactProbability.probability;
}

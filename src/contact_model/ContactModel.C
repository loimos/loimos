/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "../loimos.decl.h"
#include "../Location.h"
#include "../Event.h"
#include "ContactModel.h"

#include <vector>
#include <random>

const double DEFAULT_CONTACT_PROBABILITY = 0.5;

ContactModel::ContactModel() {
  unitDistrib = std::uniform_real_distribution<>(0.0, 1.0);
  contactProbabilityIndex = -1;
}

void ContactModel::setGenerator(std::default_random_engine *generator) {
  this->generator = generator;
}

// We don't need to do anything here, since we don't do anything
// location-specific
void ContactModel::computeLocationValues(Location& location) {}

// Just use a constant probability
bool ContactModel::madeContact(
  const Event& susceptibleEvent,
  const Event& infectiousEvent,
  Location& location
) {
  return unitDistrib(*generator) < DEFAULT_CONTACT_PROBABILITY;
}

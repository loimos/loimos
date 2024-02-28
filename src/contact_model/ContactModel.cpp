/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "../loimos.decl.h"
#include "../Location.h"
#include "../Event.h"
#include "../readers/AttributeTable.h"
#include "ContactModel.h"
#include "MinMaxAlphaModel.h"

#include <vector>
#include <random>

// This ought to be handled by the loimos definitions, but unofrtunately it
// seems we need to put this here explicitly for now
extern int contactModelType;

const double DEFAULT_CONTACT_PROBABILITY = 0.5;

ContactModel::ContactModel(const AttributeTable &attrs) {
  unitDistrib = std::uniform_real_distribution<>(0.0, 1.0);
  contactProbabilityIndex = -1;
}

// We don't need to do anything here, since we don't do anything
// location-specific
void ContactModel::computeLocationValues(Location *location) {}

// Just use a constant probability
bool ContactModel::madeContact(const Event &susceptibleEvent,
  const Event &infectiousEvent, Location *location) {
  return unitDistrib(*location->getGenerator()) < DEFAULT_CONTACT_PROBABILITY;
}

double ContactModel::getContactProbability(const Location &location) const {
  return DEFAULT_CONTACT_PROBABILITY;
}

ContactModel *createContactModel(int contactModelType, const AttributeTable &attrs) {
  if (static_cast<int>(ContactModelType::constant_probability) == contactModelType) {
    return new ContactModel(attrs);

  } else if (static_cast<int>(ContactModelType::min_max_alpha) == contactModelType) {
    return new MinMaxAlphaModel(attrs);
  }
}

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

extern int contactModelType;

const double DEFAULT_CONTACT_PROBABILITY = 0.5;
const unsigned int MIN = 5;
const unsigned int MAX = 40;
const unsigned int ALPHA = 1000;

ContactModel::ContactModel() {
  unitDistrib = std::uniform_real_distribution<>(0.0, 1.0);
  contactProbabilityIndex = -1;
}

void ContactModel::setGenerator(std::default_random_engine *generator) {
  this->generator = generator;
}

void ContactModel::computeLocationValues(Location& location) {
  if (contactModelType == (int) ContactModelType::min_max_alpha) {
    std::vector<union Data> &data = location.getDataField();
    
    if (-1 == contactProbabilityIndex) {
      contactProbabilityIndex = (int) data.size();
    }

    union Data contactProbability;
    double max_visits = (double) data[LOCATIONS_MAX_VISITS_INDEX].uint_32;
    contactProbability.probability = fmin(
      1,
      (MIN + (MAX - MIN) * (1.0 - exp(-max_visits / ALPHA))) / (max_visits - 1)
    );

    data.push_back(contactProbability);
  }
}

bool ContactModel::madeContact(
  const Event& susceptibleEvent,
  const Event& infectiousEvent,
  Location& location
) {
  if (contactModelType == (int) ContactModelType::min_max_alpha) {
    union Data contactProbability = 
      location.getDataField()[contactProbabilityIndex];
    return unitDistrib(*generator) < contactProbability.probability;
  } else {
    return unitDistrib(*generator) < DEFAULT_CONTACT_PROBABILITY;
  }
}

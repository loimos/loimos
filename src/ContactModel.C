#include "ContactModel.h"

#include <random>

const double CONTACT_PROBABILITY = 0.5;

ContactModel::ContactModel() {
  unitDistrib = std::uniform_real_distribution<>(0.0, 1.0);
}

void ContactModel::setGenerator(std::default_random_engine *generator) {
  this->generator = generator;
}

bool ContactModel::madeContact(int susceptibleIdx, int infectiousIdx) {
  return unitDistrib(*generator) < CONTACT_PROBABILITY;
}

/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef CONTACT_MODEL_CONTACTMODEL_H_
#define CONTACT_MODEL_CONTACTMODEL_H_

// Forward declaration to help with includes
class ContactModel;

#include "../Location.h"
#include "../Event.h"

#include <random>

// This is the default implmenetation, which uses a constant contact
// probability for every pair of people at every location. Other implmentations
// should extend this class. Note that this is NOT an abstract class because
// we want to use it for variables and return types
class ContactModel {
 protected:
  // These are protected rather than private so child classes can use them
  std::default_random_engine *generator;
  std::uniform_real_distribution<> unitDistrib;
  int contactProbabilityIndex;

 public:
  ContactModel();
  // Explicitly create other default constructors and assignment operators
  ContactModel(const ContactModel &other) = default;
  ContactModel& operator=(const ContactModel &other) = default;
  ContactModel(ContactModel &&other) = default;
  ContactModel& operator=(ContactModel &&other) = default;
  void setGenerator(std::default_random_engine *generator);
  // Calculates any location-specific values and stores them as new
  // attributes of the location
  virtual void computeLocationValues(Location *location);
  // Returns whether or not two people at the same location make contact
  // (will probably need to mess with the arguments once we start
  // implementing more complex models)
  virtual bool madeContact(const Event& susceptibleEvent,
    const Event& infectiousEvent, const Location& location);
};

// This enum provides an easy way of specifying which contact model to use.
// Each enum value should be named after a class which extends ContactModel
enum class ContactModelType { constant_probability, min_max_alpha };

// This creates a new instance of the contact model class indicated by
// the global variable contactModelType
ContactModel *createContactModel();

#endif  // CONTACT_MODEL_CONTACTMODEL_H_

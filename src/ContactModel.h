/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __ContactModel_H__
#define __ContactModel_H__

// Forward declaration to help with includes
class ContactModel;

#include "Event.h"
#include "Location.h"

#include <random>

// May add more eventually, or split them into different classes; for now,
// though, we only need one class since 
enum class ContactModelType { constant_probability, min_max_alpha };

// Note that this is just an example implemntation, this should eventually be
// split into an abstract class and an implementation of that API
class ContactModel {

  private:
    std::default_random_engine *generator;
    std::uniform_real_distribution<> unitDistrib;
    int contactProbabilityIndex;
  public:
    ContactModel();
    void setGenerator(std::default_random_engine *generator);
    // Calculates any location-specific values and stores them as new
    // attributes of the location
    void computeLocationValues(Location &location);
    // Returns whether or not two people at the same location make contact
    // (will probably need to mess with the arguments once we start
    // implementing more complex models)
    bool madeContact(
      const Event& susceptibleEvent,
      const Event& infectiousEvent,
      Location& location
    );
};

#endif //__ContactModel_H__

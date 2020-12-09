/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __ContactModel_H__
#define __ContactModel_H__

#include "Event.h"

#include <random>

// Note that this is just an example implemntation, this should eventually be
// split into an abstract class and an implementation of that API
class ContactModel {
  private:
    std::default_random_engine *generator;
    std::uniform_real_distribution<> unitDistrib;
  public:
    ContactModel();
    void setGenerator(std::default_random_engine *generator);
    // Returns whether or not two people at the same location make contact
    // (will probably need to mess with the arguments once we start
    // implementing more complex models)
    bool madeContact(
      const Event& susceptibleEvent, const Event& infectiousEvent
    );
};

#endif //__ContactModel_H__

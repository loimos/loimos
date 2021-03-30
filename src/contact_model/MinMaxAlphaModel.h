/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __MinMaxAlphaModel_H__
#define __MinMaxAlphaModel_H__

#include "../Location.h"
#include "../Event.h"
#include "ContactModel.h"

#include <random>

// Each location has a fixed probability of two people making contact which
// depends on the maximum number of simultaneous visits to that location,
// as well as the constants MIN, MAX, and ALPHA
class MinMaxAlphaModel : public ContactModel {

  private:
    // Specifies where to look for the attribute we create to store each
    // location's contact probability
    int contactProbabilityIndex;
  
  public:
    // We need to re-declare all of these methods from ContactModel so
    // we can override them
    MinMaxAlphaModel();
    void computeLocationValues(Location& location) override;
    bool madeContact(
      const Event& susceptibleEvent,
      const Event& infectiousEvent,
      Location& location
    ) override;
};

#endif //__MinMaxAlphaModel_H__

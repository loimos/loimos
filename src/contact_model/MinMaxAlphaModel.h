/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef CONTACT_MODEL_MINMAXALPHAMODEL_H_
#define CONTACT_MODEL_MINMAXALPHAMODEL_H_

#include "../Location.h"
#include "../Event.h"
#include "../readers/AttributeTable.h"
#include "ContactModel.h"

#include <random>

// Each location has a fixed probability of two people making contact which
// depends on the maximum number of simultaneous visits to that location,
// as well as the constants MIN, MAX, and ALPHA
class MinMaxAlphaModel : public ContactModel {
 private:
  // Specifies where to look for the attribute we create to store each
  // location's contact probability
  int maxSimVisitsIndex;

 public:
  // We need to re-declare all of these methods from ContactModel so
  // we can override them
  MinMaxAlphaModel(const AttributeTable &attrs);
  void computeLocationValues(Location *location) override;
  bool madeContact(const Event &susceptibleEvent,
    const Event& infectiousEvent, Location *location) override;
  double getContactProbability(const Location &location) const override;
};

#endif  // CONTACT_MODEL_MINMAXALPHAMODEL_H_

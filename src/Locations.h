/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LOCATIONS_H_
#define LOCATIONS_H_

#include "Location.h"
#include "DiseaseModel.h"
#include "Location.h"
#include "contact_model/ContactModel.h"

#include <vector>
#include <set>
#include <string>

class Locations : public CBase_Locations {
 private:
  int numLocalLocations;
  std::vector<Location> locations;
  std::default_random_engine generator;
  DiseaseModel *diseaseModel;
  ContactModel *contactModel;
  int day;

 public:
  explicit Locations(std::string scenarioPath);
  explicit Locations(CkMigrateMessage *msg);
  void pup(PUP::er &p);  // NOLINT(runtime/references)
  void ReceiveVisitMessages(VisitMessage visitMsg);
  void ComputeInteractions();  // calls ReceiveInfections
  // Load location data from CSV.
  void loadLocationData(std::string scenarioPath);
  #ifdef ENABLE_LB
  void ResumeFromSync();
  #endif  // ENABLE_LB
};

#endif  // LOCATIONS_H_

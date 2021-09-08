/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __LOCATIONS_H__
#define __LOCATIONS_H__

#include "Location.h" 
#include "DiseaseModel.h"
#include "Location.h" 
#include "contact_model/ContactModel.h"

#include <vector>
#include <set>

class Locations : public CBase_Locations {
  private:
    int numLocalLocations;
    int locationsInitialized;
    std::vector<Location> locations;
    std::default_random_engine generator;
    DiseaseModel *diseaseModel;
    ContactModel *contactModel;

  public:
    Locations();
    void ReceiveLocationSetup(DataInterfaceMessage *msg);

    void ReceiveVisitMessages(VisitMessage visitMsg);
    void ComputeInteractions(); // calls ReceiveInfections
  };

#endif // __LOCATIONS_H__

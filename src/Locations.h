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
#include "ContactModel.h"

#include <vector>
#include <set>
#include <functional>

class Locations : public CBase_Locations {
  private:
    int numLocalLocations;
    std::vector<Location> locations;
    std::default_random_engine generator;
    DiseaseModel *diseaseModel;
  
    ContactModel *contactModel;
    // Determines whether two people at the same location make contact
    std::function<void(std::default_random_engine,int,int)> madeContact;

  public:
    Locations();
    void ReceiveVisitMessages(
      int locationIdx,
      int personIdx,
      int personState,
      int visitStart,
      int visitEnd
    );
    void ComputeInteractions(); // calls ReceiveInfections
};

#endif // __LOCATIONS_H__

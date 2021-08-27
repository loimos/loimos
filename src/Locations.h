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

#include <hypercomm/routing.hpp>
#include <hypercomm/aggregation.hpp>

class Locations : public CBase_Locations {
  private:
    int numLocalLocations;
    std::vector<Location> locations;
    std::default_random_engine generator;
    DiseaseModel *diseaseModel;
    ContactModel *contactModel;
    int day;

    using aggregator_t = aggregation::array_aggregator<
      aggregation::direct_buffer, aggregation::routing::direct, InteractionMessage>;
    std::shared_ptr<aggregator_t> aggregator;
    bool useAggregator;

  public:
    Locations();
    Locations(CkMigrateMessage *msg);
    void pup(PUP::er &p);
    void CreateAggregator(bool useAggregator, size_t bufferSize, double threshold,
        double flushPeriod, bool nodeLevel, CkCallback cb);
    void ReceiveVisitMessages(VisitMessage visitMsg);
    void ComputeInteractions(); // calls ReceiveInfections
    
    // Load location data from CSV.
    void loadLocationData();

    #ifdef ENABLE_LB
    void ResumeFromSync();
    #endif // ENABLE_LB
};

#endif // __LOCATIONS_H__

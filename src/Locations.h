/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LOCATIONS_H_
#define LOCATIONS_H_

#include "Types.h"
#include "Location.h"
#include "Scenario.h"
#include "Location.h"
#include "contact_model/ContactModel.h"

#include <vector>
#include <set>
#include <string>
#include <unordered_map>
#include <iostream>

class Locations : public CBase_Locations {
 private:
  Id numLocalLocations;
  Id firstLocalLocationIdx;
  std::vector<Location> locations;
  Scenario *scenario;
  std::ofstream *interactionsFile;
  Counter exposureDuration;
  Counter expectedExposureDuration;
  int day;

  std::unordered_map<Id, PersonState> visitorStates;

  // For random generation.
  static std::uniform_real_distribution<> unitDistrib;

  void loadLocationData(std::string scenarioPath);
  void loadVisitData(std::ifstream *activityData);

 public:
  explicit Locations(int seed, std::string scenarioPath);
  explicit Locations(CkMigrateMessage *msg);
  void pup(PUP::er &p);  // NOLINT(runtime/references)
  void ReceiveVisitSchedule(VisitScheduleMessage msg);
  void SendExpectedVisitors();
  void ReceiveVisitorStates(PersonStatesMessage msg);
  void QueueVisits();
  static void queueVisitsImpl(
    std::vector<Location>& locations,
    const Scenario* scenario,
    int day,
    const std::unordered_map<Id, PersonState>& visitorStates
  );
  void ReceiveVisitMessages(VisitMessage visitMsg);
  void ComputeInteractions();  // calls ReceiveInfections
  void ReceiveIntervention(PartitionId interventionIdx);
  void ReceiveVisitIntervention(VisitInterventionMessage msg);
  #ifdef ENABLE_LB
  void ResumeFromSync();
  #endif  // ENABLE_LB
};

#endif  // LOCATIONS_H_

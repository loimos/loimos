/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PEOPLE_H_
#define PEOPLE_H_

#include "Types.h"
#include "DiseaseModel.h"
#include "Interaction.h"
#include "Person.h"
#include "Message.h"
#include "intervention_model/Intervention.h"

#include <functional>
#include <random>
#include <vector>
#include <tuple>
#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <set>

#define LOCATION_LAMBDA 5.2

class People : public CBase_People {
 private:
  int day;
  Id numLocalPeople;
  Counter totalVisitsForDay;
  std::vector<Person> people;
  std::unordered_set<Id> inactiveDestinations;
  DiseaseModel *diseaseModel;
  std::vector<Id> stateSummaries;
  std::ofstream *exposuresFile;
  std::ofstream *transitionsFile;

  void ProcessInteractions(Person *person);
  void UpdateDiseaseState(Person *person);
  void loadPeopleData(std::string scenarioPath);
  void loadVisitData(std::ifstream *activityData);
  void SendVisitMessages(VisitTest test);

 public:
  explicit People(int seed, std::string scenarioPath);
  explicit People(CkMigrateMessage *msg);
  void pup(PUP::er &p);  // NOLINT(runtime/references)
  void generatePeopleData(Id firstLocalPersonIndex, int seed);
  void generateVisitData();
  void SendVisitMessages();
  void DeactivateDestination(Id locationIndex);
  void ActivateDestination(Id locationIndex);
  double getTransmissionModifier(const Person &person);
  void ReceiveInteractions(InteractionMessage interMsg);
  void EndOfDayStateUpdate();
  void SendStats();
  void ReceiveIntervention(int interventionIdx);
  #ifdef ENABLE_LB
  void ResumeFromSync();
  #endif  // ENABLE_LB
};

#endif  // PEOPLE_H_

/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PEOPLE_H_
#define PEOPLE_H_

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

#define LOCATION_LAMBDA 5.2

class People : public CBase_People {
 private:
  int numLocalPeople;
  int day;
  int totalVisitsForDay;

  std::vector<Person> people;
  std::default_random_engine generator;
  DiseaseModel *diseaseModel;
  std::vector<int> stateSummaries;

  void ProcessInteractions(Person *person);
  void loadPeopleData(std::string scenarioPath);
  void loadVisitData(std::ifstream *activityData);

 public:
  explicit People(std::string scenarioPath);
  explicit People(CkMigrateMessage *msg);
  void pup(PUP::er &p);  // NOLINT(runtime/references)
  void SendVisitMessages();
  void SyntheticSendVisitMessages();
  void RealDataSendVisitMessages();
  void ReceiveInteractions(InteractionMessage interMsg);
  void EndOfDayStateUpdate();
  void SendStats();
  void ReceiveIntervention(std::shared_ptr<Intervention> v);
  #ifdef ENABLE_LB
  void ResumeFromSync();
  #endif  // ENABLE_LB
};

#endif  // PEOPLE_H_

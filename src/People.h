/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __PEOPLE_H__
#define __PEOPLE_H__

#include "DiseaseModel.h"
#include "Interaction.h"
#include "Person.h"
#include "Message.h"
#include "Aggregator.h"

#include <random>
#include <vector>
#include <tuple>
#include <iostream>
#include <fstream>
#include <memory>

#define LOCATION_LAMBDA 5.2

class People : public CBase_People {
  private:
    int numLocalPeople;
    int day;
    int newCases;
    int totalVisitsForDay;

    std::vector<Person> people;
    std::default_random_engine generator;
    DiseaseModel *diseaseModel;
    std::vector<int> stateSummaries;
    bool useAggregator;
    std::shared_ptr<Aggregator> aggregator;

    void ProcessInteractions(Person &person);
    void loadPeopleData();
    void loadVisitData(std::ifstream *activityData);
  public:
    People();
    People(CkMigrateMessage *msg);
    void pup(PUP::er &p);
    void CreateAggregator(bool useAggregator, size_t bufferSize,
        double threshold, double flushPeriod, bool nodeLevel, CkCallback cb);
    void SendVisitMessages(); 
    void SyntheticSendVisitMessages();
    void RealDataSendVisitMessages();
    void ReceiveInteractions(InteractionMessage interMsg);
    void EndOfDayStateUpdate();
    void SendStats();
    #ifdef ENABLE_LB
    void ResumeFromSync();
    #endif // ENABLE_LB
};

#endif // __PEOPLE_H__

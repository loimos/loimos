/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __PEOPLE_H__
#define __PEOPLE_H__

#include "DiseaseModel.h"
#include "Person.h"

#include <random>
#include <vector>
#include <tuple>
#include <iostream>
#include <fstream>
#define LOCATION_LAMBDA 5.2

class People : public CBase_People {
  private:
    int numLocalPeople;
    int day;
    int newCases;
    
    std::ifstream *activityData;
    std::vector<Person *> people;
    std::default_random_engine generator;
    DiseaseModel *diseaseModel;
    void loadPeopleData();
  public:
    People();
    void SendVisitMessages(); 
    void ReceiveInfections(int personIdx);
    void EndofDayStateUpdate();
};

#endif // __PEOPLE_H__

/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __PEOPLE_H__
#define __PEOPLE_H__

#include "Person.h"

#include <random>
#include <vector>
#include <tuple>
#include "DiseaseModel.h"

#define LOCATION_LAMBDA 5.2

class People : public CBase_People {
  private:
    int numLocalPeople;
    int day;
    int newCases;
    std::vector<Person> people;
    std::default_random_engine generator;
    DiseaseModel* diseaseModel;
  public:
    People();
    void SendVisitMessages(); 
    void ReceiveInfections(int personIdx);
    void EndofDayStateUpdate();
    int day;
    int newCases;
    float MAX_RANDOM_VALUE;
};

#endif // __PEOPLE_H__

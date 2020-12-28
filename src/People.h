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

#include <random>
#include <vector>
#include <tuple>
#define LOCATION_LAMBDA 5.2

class People : public CBase_People {
  private:
    int numLocalPeople;
    int day;
    int newCases;
    std::ifstream *activity_stream;
    std::vector<Person *> people;
    std::default_random_engine generator;
    DiseaseModel *diseaseModel;

    void ProcessInteractions(Person &person);
  public:
    People();
    void SendVisitMessages(); 
    void ReceiveInteractions(
      int personIdx,
      const std::vector<Interaction> &interactions
    );
    void EndofDayStateUpdate();
};

#endif // __PEOPLE_H__

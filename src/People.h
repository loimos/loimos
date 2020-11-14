/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __PEOPLE_H__
#define __PEOPLE_H__

#include "DiseaseModel.h"

#include <random>
#include <vector>
#include <tuple>
#include <iostream>
#include <fstream>
#define LOCATION_LAMBDA 5.2

// This is just a bundle of information that we don't need to
// guarentee any constraints on, hence why this is a stuct rather than
// a class (move this to a seperate file if we ever need to add any methods)
struct Person {
  // the person's curent state in the disease model
  int unique_id;
  int state;
  // how long until the person transitions to their next state
  int secondsLeftInState;
  std::vector<uint32_t> interactions_by_day;

  // std::vector<void *> attributes;
};

class People : public CBase_People {
  private:
    int numLocalPeople;
    int day;
    int newCases;
    std::ifstream *activity_stream;
    std::vector<Person> people;
    std::default_random_engine generator;
    DiseaseModel *diseaseModel;
    void loadPersonFromCSV(int personIdx, std::string *data);
  public:
    People();
    void SendVisitMessages(); 
    void ReceiveInfections(int personIdx);
    void EndofDayStateUpdate();
};

#endif // __PEOPLE_H__

/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __PEOPLE_H__
#define __PEOPLE_H__

#include "DiseaseModel.h"
#include "Interaction.h"

#include <random>
#include <vector>
#include <tuple>
#define LOCATION_LAMBDA 5.2

// This is just a bundle of information that we don't need to
// guarentee any constraints on, hence why this is a stuct rather than
// a class (move this to a seperate file if we ever need to add any methods)
struct Person {
  // the person's curent state in the disease model
  int unique_id;
  int state;
  // How long until the person transitions to their next state
  int secondsLeftInState;
  // If this is a susceptible person, this is a list of all of their
  // interactions with infectious people in the past day
  std::vector<Interaction> interactions;

  // 
  std::vector<uint32_t> interactionsByDay;
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

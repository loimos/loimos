/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __PEOPLE_H__
#define __PEOPLE_H__

#include <random>
#include <vector>

#define LOCATION_LAMBDA 5.2

class People : public CBase_People {
  private:
    int numLocalPeople;
    std::vector<char> peopleState;
    std::default_random_engine generator;
  public:
    People();
    void SendVisitMessages(); 
    void ReceiveInfections(int personIdx, char state);
    void EndofDayStateUpdate();
};

#endif // __PEOPLE_H__

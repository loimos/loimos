/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __PEOPLE_H__
#define __PEOPLE_H__

#include <random>
#include <vector>
#include <tuple>

#define LOCATION_LAMBDA 5.2

class People : public CBase_People {
  private:
    int numLocalPeople;
    std::vector<std::tuple<int, int>> peopleState;
    std::vector<int> stateSummations;
    std::default_random_engine generator;
    float MAX_RANDOM_VALUE;
  public:
    People();
    void SendVisitMessages(); 
    void ReceiveInfections(int personIdx, bool trigger_infection);
    void EndofDayStateUpdate();
};

#endif // __PEOPLE_H__

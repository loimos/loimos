/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SCHEDULE_H__
#define __SCHEDULE_H__

#include "../Person.h"

#include <random>
#include <vector>
#include <iostream>

// This is a minimal implemetation of a schedule, which doesn't actually send
// any messages. This is mainly intended as a parent class for more practical/
// realistic schedules, and the only reason it's not abstract is because we
// need to be able to make variables of this type.
class Schedule {
  private:
    // For the most part, schedules should be periodic, so if they're random
    // the easiest way to achieve that while storing minimal information is to
    // reset the seed for the random generator every so often
    int seed;
    std::default_random_engine generator;
    std::ifstream *activityData;
  public:
    Schedule();
    // Updates the seed we use for our random generator
    void setSeed(const int seed);
    // Updates the activity data file we might read data from
    void setActivityData(std::ifstream *activityData);
    // This is the main interface method for this class; this send out visit
    // messages for all of the visits in the schedules we generate for the
    // given people
    void sendVisitMessages(const std::vector<Person> &people);
};

#endif // __SCHEDULE_H__

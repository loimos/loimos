/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SCHEDULE_H__
#define __SCHEDULE_H__

#include "../Person.h"
#include "../DiseaseModel.h"

#include <random>
#include <vector>
#include <iostream>

// This is a minimal implemetation of a schedule, which doesn't actually send
// any messages (or do anythign else, really). This is mainly intended as a
// parent class for actual schedule generators/readers, and the only reason
// it's not abstract is because we need to be able to make variables of this
// type.
class Schedule {
  public:
    // Updates the seed we use for our random generator (if any)
    virtual void setSeed(const int seed);
    // Updates the activity data file we might read data from (if any)
    virtual void setActivityData(std::ifstream *activityData);
    // This is the main interface method for this class; this send out visit
    // messages for all of the visits in the schedules we generate for the
    // given people
    virtual void sendVisitMessages(
      const std::vector<Person> &people,
      const DiseaseModel *diseaseModel,
      const int peopleChareIndex,
      const int day
    );
};

// This enum rpovide sna easy way of specifying which scheule reader/generator
// to use. Each enum value should be named after a type which extends Schedule
enum class ScheduleType { syntheticSchedule, filedSchedule };

// This creates a new instance of the scheule class indicated by the global
// variable syntheticRun
Schedule *createSchedule();

#endif // __SCHEDULE_H__

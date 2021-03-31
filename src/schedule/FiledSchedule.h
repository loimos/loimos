/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __FILED_SCHEDULE_H__
#define __FILED_SCHEDULE_H__

#include "Schedule.h"
#include "../Person.h"
#include "../DiseaseModel.h"

#include <random>
#include <vector>
#include <iostream>

class FiledSchedule : public Schedule {
  private:
    std::ifstream *activityData;
  public:
    // Updates the activity data file we might read data from (if any)
    void setActivityData(std::ifstream *activityData) override;
    // This is the main interface method for this class; this send out visit
    // messages for all of the visits in the schedules we generate for the
    // given people
    void sendVisitMessages(
      const std::vector<Person> &people,
      const DiseaseModel *diseaseModel,
      const int peopleChareIndex,
      const int day
    ) override;
};

#endif // __FILED_SCHEDULE_H__

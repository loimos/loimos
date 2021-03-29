/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "../loimos.decl.h"
#include "FiledSchedule.h"
#include "../Person.h"
#include "../readers/DataReader.h"
#include "../DiseaseModel.h"

#include <vector>
#include <iostream>

void FiledSchedule::setActivityData(std::ifstream *activityData) {
  this->activityData = activityData;
}

void FiledSchedule::sendVisitMessages(
  const std::vector<Person> &people,
  const DiseaseModel *diseaseModel,
  const int peopleChareIndex,
  const int day
) {
  int numVisits, personIdx, locationIdx, locationSubset;
  // Send of activities for each person.
  int nextDaySecs = (day + 1) * DAY_LENGTH;
  int numLocalPeople = (int) people.size();
  for (int localPersonId = 0; localPersonId < numLocalPeople; localPersonId++) {
    // Seek to correct position in file.
    uint32_t seekPos = people[localPersonId].interactionsByDay[day];
    if (seekPos == EMPTY_VISIT_SCHEDULE)
      continue;
    activityData->seekg(seekPos, std::ios_base::beg);

    // Start reading
    int personId = -1; 
    int locationId = -1;
    int visitStart = -1;
    int visitDuration = -1;
    std::tie(personId, locationId, visitStart, visitDuration) = DataReader<Person>::parseActivityStream(activityData, diseaseModel->activityDef, NULL);

    // Seek while same person on same day.
    while(personId == people[localPersonId].uniqueId && visitStart < nextDaySecs) {
      // Find process that owns that location.
      locationSubset = getPartitionIndex(
          locationId,
          numLocations,
          numLocationPartitions,
          firstLocationIdx
      );

      // printf("Person %d visited %d at %d for %d. They have %d\n", personIdx, locationIdx, visitStart, visitDuration, people[localPersonId].state);
      // Send off the visit message.
      VisitMessage visitMsg(locationId, personId, people[localPersonId].state, visitStart, visitStart + visitDuration);
      locationsArray[locationSubset].ReceiveVisitMessages(visitMsg);
      std::tie(personId, locationId, visitStart, visitDuration) = DataReader<Person>::parseActivityStream(activityData, diseaseModel->activityDef, NULL);
    }
  }
}

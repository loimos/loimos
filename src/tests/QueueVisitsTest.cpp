#include "gtest/gtest.h"

#include "../decl.h"
#include "../Locations.h"
#include "../Location.h"
#include "../Message.h"
#include "../Scenario.h"
#include "../DiseaseModel.h"
#include "../Types.h"
#include "../Defs.h"
#include "../DiscreteEventSimulation.h"

class FakeDiseaseModel : public DiseaseModel {
public:
  // // false for simplicity -- only needed if we want to test SC
  // bool isInfectious(DiseaseState) const override {
  //   return false;
  // }
};

struct FakeScenario : public Scenario {
  FakeScenario() {
    // numDaysWithDistinctVisits = 1;
    diseaseModel = &fakeDiseaseModel;
  }

  FakeDiseaseModel fakeDiseaseModel;
};

// static Locations makeBasicLocationsSetup() {
//   FakeScenario* scenario = new FakeScenario();

//   Locations locs;
//   locs.scenario = scenario;
//   locs.day = 0;

//   // 1 location
//   Location location;
//   locs.locations.push_back(location);

//   // visitsByDay must be sized
//   locs.locations[0].visitsByDay.resize(1);

//   // 1 active visit
//   VisitMessage visit;
//   visit.personIdx = 0;
//   visit.visitStart = 10;
//   visit.visitEnd = 20;
//   visit.deactivatedBy = 0; // anything other than -1 is 'active'

//   locs.locations[0].visitsByDay[0].push_back(visit);

//   PersonState ps;
//   ps.state = 1;
//   ps.transmissionModifier = 1.0;

//   locs.visitorStates.emplace(0, ps);

//   return locs;
// }


TEST(LocationsTest, QueueVisitsImpl_CreatesArrivalAndDepartureEvents) {
  FakeScenario scenario;
  int day = 0;

  std::vector<Location> locations;
  locations.emplace_back();  // 1 location

  // visitsByDay must be sized
  locations[0].visitsByDay.resize(1);

  // 1 active visit
  VisitMessage visit;
  visit.personIdx = 0;
  visit.visitStart = 10;
  visit.visitEnd = 20;
  visit.deactivatedBy = 0;  // active (anything not -1 is active)

  locations[0].visitsByDay[0].push_back(visit);

  std::unordered_map<Id, PersonState> visitorStates;

  PersonState ps;
  ps.state = 1;
  ps.transmissionModifier = 1.0;

  visitorStates.emplace(0, ps);

  queueVisitsImpl(
      locations,
      &scenario,
      day,
      visitorStates
  );

  const auto& events = locations[0].events;

  ASSERT_EQ(events.size(), 2);

  // Arrival event
  EXPECT_EQ(events[0].type, ARRIVAL);
  EXPECT_EQ(events[0].personIdx, 0);
  EXPECT_EQ(events[0].scheduledTime, 10);

  // Departure event
  EXPECT_EQ(events[1].type, DEPARTURE);
  EXPECT_EQ(events[1].personIdx, 0);
  EXPECT_EQ(events[1].scheduledTime, 20);
}

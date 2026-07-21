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

// SIR model
class FakeDiseaseModel : public DiseaseModel {
 public:
  bool isSusceptible(DiseaseState ds) const Override {
    return ds == 0;
  }

  // State 1 is I
  bool isInfectious(DiseaseState ds) const override {
    return ds == 1;
  }

  double DiseaseModel::getPropensity(DiseaseState susceptibleState,
    DiseaseState infectiousState, Time startTime, Time endTime,
    double susceptibility, double infectivity)
    const {

    Time dt = endTime - startTime;
    return dt * susceptibility * infectivity;
  }
};

struct FakeScenario : public Scenario {
  FakeScenario() {
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

  // 1 susceptible visit
  VisitMessage visit;
  visit.personIdx = 0;
  visit.visitStart = 10;
  visit.visitEnd = 20;
  visit.deactivatedBy = 0;  // active (anything not -1 is active)
  visit.personState = 0;    // Susceptible

  locations[0].visitsByDay[0].push_back(visit);

  std::unordered_map<Id, PersonState> visitorStates;

  PersonState ps;
  ps.state = 0;
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

TEST(LocationsTest, QueueVisitsImpl_CreatesArrivalAndDepartureEvents) {
  FakeScenario scenario;
  int day = 0;

  std::vector<Location> locations;
  locations.emplace_back();  // 1 location

  // visitsByDay must be sized
  locations[0].visitsByDay.resize(1);

  std::unordered_map<Id, PersonState> visitorStates;

  // 1 susceptible visit
  VisitMessage visit;
  visit.personIdx = 0;
  visit.visitStart = 10;
  visit.visitEnd = 20;
  visit.deactivatedBy = 0;  // active (anything not -1 is active)

  visitorStates.emplace(0, { .state = 0, .transmissionModifier = 1.0 });
  locations[0].visitsByDay[0].push_back(visit);

  // 2 infectious visits
  // 1 overlaps
  VisitMessage visit1;
  visit1.personIdx = 1;
  visit1.visitStart = 15;
  visit1.visitEnd = 25;
  visit1.deactivatedBy = 0;  // active (anything not -1 is active)

  visitorStates.emplace(1, { .state = 1, .transmissionModifier = 1.0 });
  locations[0].visitsByDay[0].push_back(visit1);

  // 1 doesn't overlap
  VisitMessage visit2;
  visit2.personIdx = 2;
  visit2.visitStart = 20;
  visit2.visitEnd = 30;
  visit2.deactivatedBy = 0;  // active (anything not -1 is active)

  visitorStates.emplace(2, { .state = 1, .transmissionModifier = 1.0 });
  locations[0].visitsByDay[0].push_back(visit2);

  // 1 recovered
  VisitMessage visit3;
  visit3.personIdx = 3;
  visit3.visitStart = 20;
  visit3.visitEnd = 30;
  visit3.deactivatedBy = 0;  // active (anything not -1 is active)

  visitorStates.emplace(3, { .state = 2, .transmissionModifier = 1.0 });
  locations[0].visitsByDay[0].push_back(visit3);

  queueVisitsImpl(
      locations,
      &scenario,
      day,
      visitorStates
  );

  std::unordered_map<Id, std::vector<Interaction>> interactions;
  Counter locInters = processEvents(&locations[0], scenario, interactionsFile, thisIndex,
    &interactions);

  EXPECT_EQ(interactions.count(0), 1);
  EXPECT_EQ(interactions.count(1), 0);
  EXPECT_EQ(interactions.count(2), 0);
  EXPECT_EQ(interactions.count(3), 0);

  std::vector<Interaction> &susInters = interactions[0];
  EXPECT_EQ(locInters, 1);
  EXPECT_EQ(susInters.size(), 1);

  if (susInters.size() == 1) {
    Interaction &inter = susInters[0];

    EXPECT_EQ(inter.infectiousIdx, 1);
    EXPECT_EQ(inter.infectiousState, 1);
    EXPECT_EQ(inter.startTime, 15);
    EXPECT_EQ(inter.endTime, 20);
    EXPECT_EQ(inter.propensity, 5.0);
  }
}

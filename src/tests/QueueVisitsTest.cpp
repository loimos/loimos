#include <gtest/gtest.h>

#include "Locations.h"
#include "Location.h"
#include "VisitMessage.h"
#include "Event.h"
#include "Scenario.h"
#include "DiseaseModel.h"

class FakeDiseaseModel : public DiseaseModel {
public:
    // what is diseasestate - whats the necessary import?
  bool isInfectious(DiseaseState personState) const override {
  }
};

struct FakeScenario : public Scenario {
  FakeScenario() {
    numDaysWithDistinctVisits = 1;
    diseaseModel = &fakeDiseaseModel;
  }

  FakeDiseaseModel fakeDiseaseModel;
};


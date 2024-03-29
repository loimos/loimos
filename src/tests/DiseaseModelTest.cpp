/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "../loimos.decl.h"
#include "../DiseaseModel.h"
#include "../People.h"
#include "../Person.h"
#include "gtest/gtest.h"

/** Tests the disease model. */

namespace {

class DiseaseModelTest : public ::testing::Test {
 protected:
  std::string PATH_TO_DISEASE_MODEL =
    "../data/disease_models/validation_model.textproto";
  std::string SCENARIO_PATH = "../data/populations/validation_set";

  virtual void SetUp() {
    diseaseModel = new DiseaseModel(PATH_TO_DISEASE_MODEL, SCENARIO_PATH);
  }

  DiseaseModel *diseaseModel;
};

TEST_F(DiseaseModelTest, InfectionTest) {
  /** Tests disease model transitions. */
  Data age;
  age.int_b10 = 60;
  std::vector<Data> personData;
  personData.push_back(age);
  Person *person =
      new Person(1, diseaseModel->getHealthyState(personData), -1);

  // Person should make a transition as they have no time in state left.
  int originalState = person->state;
  std::default_random_engine generator;
  EXPECT_EQ(person->state, 3);  // Start in healthy risky.
  person->EndOfDayStateUpdate(diseaseModel, &generator);
  EXPECT_EQ(person->state, 4);  // Should transition
  EXPECT_EQ(person->secondsLeftInState, 3 * (60 * 60 * 24));
}

TEST_F(DiseaseModelTest, TestModelLoad) {
  /** Tests that the model was loaded correctly. */
  EXPECT_EQ(diseaseModel->getNumberOfStates(), 6);
  EXPECT_EQ(diseaseModel->lookupStateName(0), "healthy_safe");
  EXPECT_EQ(diseaseModel->lookupStateName(1), "infectious_safe");
}

}  // namespace

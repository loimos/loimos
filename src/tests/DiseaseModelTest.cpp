/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "../DiseaseModel.h"
#include "../Defs.h"
#include "../loimos.decl.h"
#include "../Event.h"
#include "../readers/AttributeTable.h"
#include "gtest/gtest.h"

#include <cmath>
#include <limits>
#include <memory>
#include <random>
#include <tuple>
#include <vector>

namespace
{

  AttributeTable BuildTestAttributeTable()
  {
    AttributeTable table;
    Attribute ageAttribute;
    ageAttribute.name = "age";
    ageAttribute.dataType = DataTypes::int32_;
    Data ageDefault{};
    ageDefault.int32_val = 0;
    ageAttribute.defaultValue = ageDefault;
    table.list.push_back(ageAttribute);
    return table;
  }

  class SafeRiskyDiseaseModelTest : public ::testing::Test
  {
  protected:
    void SetUp() override
    {
      attribute_table_ = BuildTestAttributeTable();
      disease_model_ = std::make_unique<DiseaseModel>(
          "../data/disease_models/safe_risky.textproto", -1.0, attribute_table_);
    }

    AttributeTable attribute_table_;
    std::unique_ptr<DiseaseModel> disease_model_;
  };

  TEST_F(SafeRiskyDiseaseModelTest, HealthyStateDependsOnAge)
  {
    std::vector<Data> person_data(1);
    person_data[0].int32_val = 30;
    EXPECT_EQ(disease_model_->getHealthyState(person_data), 3);

    person_data[0].int32_val = 80;
    EXPECT_EQ(disease_model_->getHealthyState(person_data), 0);
  }

  TEST_F(SafeRiskyDiseaseModelTest, ExposureTransitionsAreImmediate)
  {
    std::default_random_engine generator(12345);
    auto transition = disease_model_->transitionFromState(3, &generator);
    EXPECT_EQ(std::get<0>(transition), 4);
    EXPECT_EQ(std::get<1>(transition), 0);
  }

  TEST_F(SafeRiskyDiseaseModelTest, TimedTransitionReturnsDurationInSeconds)
  {
    std::default_random_engine generator(12345);
    auto transition = disease_model_->transitionFromState(4, &generator);
    EXPECT_EQ(std::get<0>(transition), 5);
    EXPECT_EQ(std::get<1>(transition), 3 * DAY_LENGTH);
  }

  TEST_F(SafeRiskyDiseaseModelTest, TerminalStatesDoNotAdvance)
  {
    std::default_random_engine generator(9876);
    auto transition = disease_model_->transitionFromState(2, &generator);
    EXPECT_EQ(std::get<0>(transition), 2);
    EXPECT_EQ(std::get<1>(transition), std::numeric_limits<Time>::max());
  }

  TEST_F(SafeRiskyDiseaseModelTest, InfectiousAndSusceptibleFlagsMatchModel)
  {
    EXPECT_TRUE(disease_model_->isSusceptible(0));
    EXPECT_FALSE(disease_model_->isSusceptible(2));
    EXPECT_TRUE(disease_model_->isInfectious(4));
    EXPECT_FALSE(disease_model_->isInfectious(5));
  }

  TEST_F(SafeRiskyDiseaseModelTest, LookupStateNamesReflectProtoOrder)
  {
    EXPECT_EQ(disease_model_->getNumberOfStates(), 6);
    EXPECT_EQ(disease_model_->lookupStateName(0), "healthy_safe");
    EXPECT_EQ(disease_model_->lookupStateName(4), "infectious_risky");
  }

  TEST(DiseaseModelPropensityTest, PropensityScalesWithDurationAndModifiers)
  {
    AttributeTable attributes = BuildTestAttributeTable();
    DiseaseModel model("../data/disease_models/covid19_onepath.textproto",
                       -1.0, attributes);

    DiseaseState susceptible_state = 0; // S_a
    DiseaseState infectious_state = 5;  // Isymp_a
    Time start = 0;
    Time end = 12 * HOUR_LENGTH;
    double susceptibility_scalar = 0.25;
    double infectivity_scalar = 0.5;

    double expected = model.model->transmissibility() * (end - start) * susceptibility_scalar * infectivity_scalar * model.model->disease_states(susceptible_state).susceptibility() * model.model->disease_states(infectious_state).infectivity() / DAY_LENGTH;

    double propensity = model.getPropensity(susceptible_state, infectious_state,
                                            start, end, susceptibility_scalar, infectivity_scalar);

    EXPECT_DOUBLE_EQ(propensity, expected);
  }

  TEST(DiseaseModelLogProbTest, LogProbabilityAccountsForOverlapDuration)
  {
    AttributeTable attributes = BuildTestAttributeTable();
    DiseaseModel model("../data/disease_models/covid19_onepath.textproto",
                       -1.0, attributes);

    Event susceptible_event{};
    susceptible_event.personState = 0; // S_a; susceptibility 1
    susceptible_event.scheduledTime = 0;

    Event infectious_event{};
    infectious_event.personState = 5; // Isymp_a; infectivity 1
    infectious_event.scheduledTime = 6 * HOUR_LENGTH;

    double expected_base = 1.0 - model.model->transmissibility() * model.model->disease_states(0).susceptibility() * model.model->disease_states(5).infectivity();

    double expected = std::log(expected_base) * (6 * HOUR_LENGTH);

    EXPECT_DOUBLE_EQ(model.getLogProbNotInfected(susceptible_event,
                                                 infectious_event),
                     expected);
  }

} // namespace

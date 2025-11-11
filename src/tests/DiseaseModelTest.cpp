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
    disease_model_.reset(new DiseaseModel(
      "../data/disease_models/safe_risky.textproto", -1.0, attribute_table_));
    }

    AttributeTable attribute_table_;
    std::unique_ptr<DiseaseModel> disease_model_;
  };

  TEST_F(SafeRiskyDiseaseModelTest, HealthyStateDependsOnAge)
  {
    const uint32_t N = 100;

    std::default_random_engine generator(12345);
  std::uniform_int_distribution<int> distribution(0, 100);

    for (int i = 0; i < N; i++)
    {
      int age = distribution(generator);
      std::vector<Data> person_data(1);
      person_data[0].int32_val = age;
      EXPECT_EQ(disease_model_->getHealthyState(person_data), age <= 50 ? 3 : 0);
    }
  }

  TEST_F(SafeRiskyDiseaseModelTest, ExposureTransitionsAreImmediate)
  {
    std::default_random_engine generator(12345);
    struct TransitionExpectation
    {
      DiseaseState from_state;
      DiseaseState next_state;
      Time duration;
    };

    const TransitionExpectation kTestCases[] = {
        {3, 4, 0},                                // Exposure transitions are immediate
        {2, 2, std::numeric_limits<Time>::max()}, // Terminal states do not advance
        {4, 5, 3 * DAY_LENGTH}};                  // Timed transition returns duration in seconds

    const size_t kNumCases = sizeof(kTestCases) / sizeof(kTestCases[0]);
    for (size_t i = 0; i < kNumCases; ++i)
    {
      const TransitionExpectation &test_case = kTestCases[i];
      const std::tuple<DiseaseState, Time> transition =
          disease_model_->transitionFromState(test_case.from_state, &generator);
      EXPECT_EQ(std::get<0>(transition), test_case.next_state);
      EXPECT_EQ(std::get<1>(transition), test_case.duration);
    }
  }

  TEST_F(SafeRiskyDiseaseModelTest, InfectiousAndSusceptibleFlagsMatchModel)
  {
    struct DiseaseFlags
    {
      DiseaseState state;
      bool infectious;
      bool susceptible;
    };

    const DiseaseFlags kExpected[] = {
        {0, false, true},  // healthy_safe
        {1, true, false},  // infectious_safe
        {2, false, false}, // recovered_safe
        {3, false, true},  // healthy_risky
        {4, true, false},  // infectious_risky
        {5, false, false}  // recovered_risky
    };

    const size_t kNumCases = sizeof(kExpected) / sizeof(kExpected[0]);
    for (size_t i = 0; i < kNumCases; ++i)
    {
      const DiseaseFlags &entry = kExpected[i];
      EXPECT_EQ(disease_model_->isInfectious(entry.state), entry.infectious);
      EXPECT_EQ(disease_model_->isSusceptible(entry.state), entry.susceptible);
    }
  }

  TEST_F(SafeRiskyDiseaseModelTest, LookupStateNamesReflectProtoOrder)
  {
    struct StateName
    {
      DiseaseState state;
      const char *name;
    };

    const StateName kExpected[] = {
        {0, "healthy_safe"},
        {1, "infectious_safe"},
        {2, "recovered_safe"},
        {3, "healthy_risky"},
        {4, "infectious_risky"},
        {5, "recovered_risky"}};

    const size_t kNumCases = sizeof(kExpected) / sizeof(kExpected[0]);
    for (size_t i = 0; i < kNumCases; ++i)
    {
      const StateName &entry = kExpected[i];
      EXPECT_EQ(disease_model_->lookupStateName(entry.state), entry.name);
    }
  }

  TEST(DiseaseModelPropensityTest, PropensityScalesWithDurationAndModifiers)
  {
    AttributeTable attributes = BuildTestAttributeTable();
    DiseaseModel model("../data/disease_models/covid19_onepath.textproto",
                       -1.0, attributes);

    const DiseaseState susceptible_state = 0; // S_a
    const DiseaseState infectious_state = 5;  // Isymp_a
    const Time start = 0;
    const Time end = 12 * HOUR_LENGTH;

    struct PropensityCase
    {
      double susceptibility;
      double infectivity;
    };

    const PropensityCase kCases[] = {
        {0.1, 0.1},
        {0.1, 1.0},
        {1.0, 0.1},
        {1.0, 1.0}};

    const double base_scale = model.model->transmissibility() * (end - start) *
                              model.model->disease_states(susceptible_state).susceptibility() *
                              model.model->disease_states(infectious_state).infectivity() /
                              DAY_LENGTH;

    const size_t kNumCases = sizeof(kCases) / sizeof(kCases[0]);
    for (size_t i = 0; i < kNumCases; ++i)
    {
      const PropensityCase &test_case = kCases[i];
      const double expected = base_scale * test_case.susceptibility * test_case.infectivity;
      const double propensity = model.getPropensity(susceptible_state, infectious_state,
                                                    start, end, test_case.susceptibility, test_case.infectivity);
      EXPECT_DOUBLE_EQ(propensity, expected);
    }
  }

  TEST(DiseaseModelLogProbTest, LogProbabilityAccountsForOverlapDuration)
  {
    // N=10 is acceptable in this case

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

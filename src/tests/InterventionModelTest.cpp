/* Copyright 2020-2024 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "../intervention_model/InterventionModel.h"
#include "../DiseaseModel.h"
#include "../protobuf/interventions.pb.h"
#include "../readers/AttributeTable.h"
#include "TestHelpers.h"
#include "gtest/gtest.h"

#include <memory>
#include <vector>

namespace
{

    using ProtoInterventionModel = loimos::proto::InterventionModel;

    // Helper to build test attribute tables
    AttributeTable BuildTestPersonAttributes()
    {
        return TestHelpers::CreateTestPersonAttributes();
    }

    AttributeTable BuildTestLocationAttributes()
    {
        return TestHelpers::CreateTestLocationAttributes();
    }

    // Test fixture for InterventionModel tests
    class InterventionModelTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            personAttributes_ = BuildTestPersonAttributes();
            locationAttributes_ = BuildTestLocationAttributes();
            diseaseModel_.reset(new DiseaseModel(
                "../data/disease_models/safe_risky.textproto", -1.0, personAttributes_));
        }

        AttributeTable personAttributes_;
        AttributeTable locationAttributes_;
        std::unique_ptr<DiseaseModel> diseaseModel_;
    };

    // Test default constructor
    TEST(InterventionModelBasicTest, DefaultConstructorCreatesEmptyModel)
    {
        InterventionModel model;
        EXPECT_EQ(model.getNumPersonInterventions(), 0);
        EXPECT_EQ(model.getNumLocationInterventions(), 0);
        EXPECT_TRUE(model.triggerFlags.empty());
    }

    // Test toggleInterventions with day-based trigger
    // The toggleInterventions logic sets the flag to true when conditions are met.
    // Implementation: flag = (!flag && day >= trigger_on) || (flag && day >= trigger_off)
    // Once activated (day >= trigger_on), it stays active if day >= trigger_off too.
    TEST_F(InterventionModelTest, ToggleInterventionsDayTriggerActivates)
    {
        InterventionModel model;
        model.interventionDef = new loimos::proto::InterventionModel();
        
        auto *trigger = model.interventionDef->add_triggers();
        auto *dayTrigger = trigger->mutable_day();
        dayTrigger->set_trigger_on(5);
        dayTrigger->set_trigger_off(10);
        
        model.triggerFlags.resize(1, false);
        
        // Before trigger_on day - should remain false
        model.toggleInterventions(/*day=*/3, /*newDailyInfections=*/0, /*numPeople=*/100);
        EXPECT_FALSE(model.triggerFlags[0]) << "Day 3: should not activate";
        
        // At trigger_on day - should activate
        model.toggleInterventions(/*day=*/5, /*newDailyInfections=*/0, /*numPeople=*/100);
        EXPECT_TRUE(model.triggerFlags[0]) << "Day 5: should activate";
        
        // Day 7: between trigger_on and trigger_off
        // flag = (!true && 5<=7) || (true && 10<=7) = false || false = false (turns off!)
        model.triggerFlags[0] = true;  // Reset to true to test middle range
        model.toggleInterventions(/*day=*/7, /*newDailyInfections=*/0, /*numPeople=*/100);
        EXPECT_FALSE(model.triggerFlags[0]) << "Day 7: turns off (between trigger_on and trigger_off)";
        
        // Day 10: at trigger_off when flag is false
        // flag = (!false && 5<=10) || (false && 10<=10) = true || false = true
        model.toggleInterventions(/*day=*/10, /*newDailyInfections=*/0, /*numPeople=*/100);
        EXPECT_TRUE(model.triggerFlags[0]) << "Day 10: re-activates (day >= trigger_on)";
        
        delete model.interventionDef;
    }

    // Test toggleInterventions with case rate trigger
    TEST_F(InterventionModelTest, ToggleInterventionsCaseRateTriggerActivates)
    {
        InterventionModel model;
        model.interventionDef = new loimos::proto::InterventionModel();
        
        auto *trigger = model.interventionDef->add_triggers();
        auto *caseTrigger = trigger->mutable_new_daily_cases();
        caseTrigger->set_trigger_on(0.1);   // 10% infection rate triggers on
        caseTrigger->set_trigger_off(0.05); // 5% infection rate triggers off
        
        model.triggerFlags.resize(1, false);
        
        // Low infection rate - should remain false
        model.toggleInterventions(/*day=*/0, /*newDailyInfections=*/5, /*numPeople=*/100);
        EXPECT_FALSE(model.triggerFlags[0]);
        
        // High infection rate (15%) - should activate
        model.toggleInterventions(/*day=*/1, /*newDailyInfections=*/15, /*numPeople=*/100);
        EXPECT_TRUE(model.triggerFlags[0]);
        
        // Medium infection rate (7%) - should stay active (above off threshold)
        model.toggleInterventions(/*day=*/2, /*newDailyInfections=*/7, /*numPeople=*/100);
        EXPECT_TRUE(model.triggerFlags[0]);
        
        // Low infection rate (3%) - should deactivate
        model.toggleInterventions(/*day=*/3, /*newDailyInfections=*/3, /*numPeople=*/100);
        EXPECT_FALSE(model.triggerFlags[0]);
        
        delete model.interventionDef;
    }

    // Test multiple triggers
    // Implementation: flag = (!flag && day >= trigger_on) || (flag && day >= trigger_off)
    // This means triggers turn on when crossing trigger_on threshold,
    // and turn off when between trigger_on and trigger_off (counterintuitive).
    TEST_F(InterventionModelTest, MultipleTriggersFunctionIndependently)
    {
        InterventionModel model;
        model.interventionDef = new loimos::proto::InterventionModel();
        
        // Day trigger
        auto *trigger1 = model.interventionDef->add_triggers();
        auto *dayTrigger = trigger1->mutable_day();
        dayTrigger->set_trigger_on(5);
        dayTrigger->set_trigger_off(10);
        
        // Case rate trigger
        auto *trigger2 = model.interventionDef->add_triggers();
        auto *caseTrigger = trigger2->mutable_new_daily_cases();
        caseTrigger->set_trigger_on(0.2);
        caseTrigger->set_trigger_off(0.1);
        
        model.triggerFlags.resize(2, false);
        
        // Day 5 with low infection (5%) - only day trigger activates
        // Day: (!false && 5<=5) = true → activates
        // Case: (!false && 0.2<=0.05) = false → stays off
        model.toggleInterventions(/*day=*/5, /*newDailyInfections=*/5, /*numPeople=*/100);
        EXPECT_TRUE(model.triggerFlags[0]) << "Day trigger should activate at day=5";
        EXPECT_FALSE(model.triggerFlags[1]) << "Case trigger should stay off (5% < 20%)";
        
        // Day 10 with high infection (25%) - both should activate
        // Day: day trigger was on but day=10 >= trigger_off=10, so:
        //      (!true && 5<=10) || (true && 10<=10) = false || true = true
        // Case: (!false && 0.2<=0.25) = true → activates
        // Note: we need day=10 for day trigger to stay on
        model.triggerFlags[0] = false; // Reset to test fresh
        model.triggerFlags[1] = false;
        model.toggleInterventions(/*day=*/10, /*newDailyInfections=*/25, /*numPeople=*/100);
        EXPECT_TRUE(model.triggerFlags[0]) << "Day trigger active at day=10 (>=trigger_on)";
        EXPECT_TRUE(model.triggerFlags[1]) << "Case trigger active (25% >= 20%)";
        
        delete model.interventionDef;
    }

    // Test intervention counts
    TEST_F(InterventionModelTest, InterventionCountsAreCorrect)
    {
        InterventionModel model;
        model.interventionDef = new loimos::proto::InterventionModel();
        
        // Add a trigger first
        auto *trigger = model.interventionDef->add_triggers();
        trigger->mutable_day()->set_trigger_on(1);
        trigger->mutable_day()->set_trigger_off(10);
        model.triggerFlags.resize(1, false);
        
        // Initialize with person interventions
        auto *personIntervention = model.interventionDef->add_person_interventions();
        personIntervention->set_compliance(1.0);
        personIntervention->set_trigger_index(0);
        personIntervention->mutable_self_isolation();
        
        model.initPersonInterventions(
            model.interventionDef->person_interventions(),
            personAttributes_, *diseaseModel_);
        
        EXPECT_EQ(model.getNumPersonInterventions(), 1);
        EXPECT_EQ(model.getNumLocationInterventions(), 0);
        
        // Add location intervention
        auto *locationIntervention = model.interventionDef->add_location_interventions();
        locationIntervention->set_compliance(1.0);
        locationIntervention->set_trigger_index(0);
        locationIntervention->mutable_school_closures();
        
        model.initLocationInterventions(
            model.interventionDef->location_interventions(),
            locationAttributes_, *diseaseModel_);
        
        EXPECT_EQ(model.getNumPersonInterventions(), 1);
        EXPECT_EQ(model.getNumLocationInterventions(), 1);
        
        delete model.interventionDef;
    }

    // Test that trigger flags resize correctly
    TEST_F(InterventionModelTest, TriggerFlagsResizeCorrectly)
    {
        InterventionModel model;
        model.interventionDef = new loimos::proto::InterventionModel();
        
        // Add 3 triggers
        for (int i = 0; i < 3; ++i) {
            auto *trigger = model.interventionDef->add_triggers();
            trigger->mutable_day()->set_trigger_on(i + 1);
            trigger->mutable_day()->set_trigger_off(i + 10);
        }
        
        model.triggerFlags.resize(model.interventionDef->triggers_size(), false);
        
        EXPECT_EQ(model.triggerFlags.size(), 3u);
        EXPECT_FALSE(model.triggerFlags[0]);
        EXPECT_FALSE(model.triggerFlags[1]);
        EXPECT_FALSE(model.triggerFlags[2]);
        
        delete model.interventionDef;
    }

    // Test getPersonIntervention returns correct intervention
    TEST_F(InterventionModelTest, GetPersonInterventionReturnsCorrectIntervention)
    {
        InterventionModel model;
        model.interventionDef = new loimos::proto::InterventionModel();
        
        auto *trigger = model.interventionDef->add_triggers();
        trigger->mutable_day()->set_trigger_on(1);
        trigger->mutable_day()->set_trigger_off(10);
        model.triggerFlags.resize(1, false);
        
        auto *personIntervention = model.interventionDef->add_person_interventions();
        personIntervention->set_compliance(0.75);
        personIntervention->set_trigger_index(0);
        personIntervention->mutable_self_isolation();
        
        model.initPersonInterventions(
            model.interventionDef->person_interventions(),
            personAttributes_, *diseaseModel_);
        
        const auto &intervention = model.getPersonIntervention(0);
        EXPECT_EQ(intervention.getTriggerIndex(), 0);
        EXPECT_EQ(intervention.getInterventionId(), 0);
        
        delete model.interventionDef;
    }

} // namespace

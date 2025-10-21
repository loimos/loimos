/* Copyright 2020-2024 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "../intervention_model/InterventionModel.h"

#include "../DiseaseModel.h"
#include "../protobuf/interventions.pb.h"
#include "../readers/AttributeTable.h"
#include "gtest/gtest.h"

#include <memory>
#include <vector>

namespace
{

    // DATASETS:
    // TODO: synthetic_small_city more likely to have school interventions; coc likely

    // testing objectives:
    // parsing/file IO
    // given parsed intervention -> basic functionality is correct
    //      vaccination reduces susceptibility
    //      school closure filters out visits to school locations
    // implementation guide:
    //      applied to a single person/locaiton (should be clear + sufficient):
    //      set probability to 0 for interventions -- see effect
    //      then set it to half, try among large experiment count to see incidence of half size in output
    //      right now, probabilities are not passed through nonlinear functions -- output should be linearly correspondent

    using ProtoInterventionModel = loimos::proto::InterventionModel;

    AttributeTable BuildTestPersonAttributes()
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

    AttributeTable BuildTestLocationAttributes()
    {
        AttributeTable table;
        Attribute schoolAttribute;
        schoolAttribute.name = "school";
        schoolAttribute.dataType = DataTypes::bool_;
        Data schoolDefault{};
        schoolDefault.bool_val = false;
        schoolAttribute.defaultValue = schoolDefault;
        table.list.push_back(schoolAttribute);
        return table;
    }

    std::shared_ptr<ProtoInterventionModel> BuildDayTriggeredDefinition()
    {
        auto definition = std::make_shared<ProtoInterventionModel>();
        auto *trigger = definition->add_triggers();
        auto *dayTrigger = trigger->mutable_day();
        dayTrigger->set_trigger_on(5);
        dayTrigger->set_trigger_off(8);

        auto *personIntervention = definition->add_person_interventions();
        personIntervention->set_compliance(1.0);
        personIntervention->set_trigger_index(0);
        personIntervention->mutable_self_isolation();

        auto *locationIntervention = definition->add_location_interventions();
        locationIntervention->set_compliance(1.0);
        locationIntervention->set_trigger_index(0);
        locationIntervention->mutable_school_closures();

        return definition;
    }

    std::shared_ptr<ProtoInterventionModel> BuildCaseRateDefinition()
    {
        auto definition = std::make_shared<ProtoInterventionModel>();
        auto *trigger = definition->add_triggers();
        auto *caseTrigger = trigger->mutable_new_daily_cases();
        caseTrigger->set_trigger_on(0.3);
        caseTrigger->set_trigger_off(0.1);

        auto *personIntervention = definition->add_person_interventions();
        personIntervention->set_compliance(1.0);
        personIntervention->set_trigger_index(0);
        personIntervention->mutable_self_isolation();

        return definition;
    }

    std::shared_ptr<ProtoInterventionModel> BuildLocationOnlyDefinition()
    {
        auto definition = std::make_shared<ProtoInterventionModel>();
        auto *trigger = definition->add_triggers();
        auto *dayTrigger = trigger->mutable_day();
        dayTrigger->set_trigger_on(2);
        dayTrigger->set_trigger_off(4);

        auto *locationIntervention = definition->add_location_interventions();
        locationIntervention->set_compliance(1.0);
        locationIntervention->set_trigger_index(0);
        locationIntervention->mutable_school_closures();

        return definition;
    }

    class InterventionModelTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            personAttributes_ = BuildTestPersonAttributes();
            locationAttributes_ = BuildTestLocationAttributes();
            diseaseModel_ = std::make_unique<DiseaseModel>(
                "../data/disease_models/safe_risky.textproto", -1.0, personAttributes_);
        }

        AttributeTable personAttributes_;
        AttributeTable locationAttributes_;
        std::unique_ptr<DiseaseModel> diseaseModel_;
    };

} // namespace

TEST_F(InterventionModelTest, ApplyInterventionsUsesCustomCallbacks)
{
    auto definition = BuildDayTriggeredDefinition();
    InterventionModel model(definition, &personAttributes_, &locationAttributes_,
                            *diseaseModel_);

    std::vector<int> peopleCalls;
    std::vector<int> locationCalls;

    model.applyInterventions(/*day=*/5, /*newDailyInfections=*/0,
                             /*numPeople=*/2,
                             [&](int idx)
                             { peopleCalls.push_back(idx); },
                             [&](int idx)
                             { locationCalls.push_back(idx); });

    ASSERT_EQ(peopleCalls.size(), 1u);
    EXPECT_EQ(peopleCalls.front(), 0);
    ASSERT_EQ(locationCalls.size(), 1u);
    EXPECT_EQ(locationCalls.front(), 0);
}

TEST_F(InterventionModelTest, CaseTriggerDisablesWhenBelowOffThreshold)
{
    auto definition = BuildCaseRateDefinition();
    InterventionModel model(definition, &personAttributes_, nullptr,
                            *diseaseModel_);

    model.setTriggerFlags({true});
    model.toggleInterventions(/*day=*/0, /*newDailyInfections=*/1,
                              /*numPeople=*/50);

    EXPECT_FALSE(model.getTriggerFlag(0));
}

TEST_F(InterventionModelTest, LocationOnlyModelNotifiesLocations)
{
    auto definition = BuildLocationOnlyDefinition();
    InterventionModel model(definition, nullptr, &locationAttributes_,
                            *diseaseModel_);

    std::vector<int> locationCalls;
    model.applyInterventions(/*day=*/2, /*newDailyInfections=*/0,
                             /*numPeople=*/10,
                             InterventionModel::NotificationFn(),
                             [&](int idx)
                             { locationCalls.push_back(idx); });

    ASSERT_EQ(locationCalls.size(), 1u);
    EXPECT_EQ(locationCalls.front(), 0);
}

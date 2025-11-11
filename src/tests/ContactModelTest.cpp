/* Copyright 2020-2025 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "../contact_model/ContactModel.h"
#include "../contact_model/MinMaxAlphaModel.h"
#include "../Location.h"
#include "../Event.h"
#include "../Defs.h"
#include "TestHelpers.h"
#include "gtest/gtest.h"

#include <random>

namespace
{

    class ContactModelTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            locationAttrs_ = TestHelpers::CreateTestLocationAttributes();
            contactModel_.reset(new ContactModel(locationAttrs_));
        }

        AttributeTable locationAttrs_;
        std::unique_ptr<ContactModel> contactModel_;
    };

    class MinMaxAlphaModelTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            locationAttrs_ = TestHelpers::CreateTestLocationAttributes();
            minMaxModel_.reset(new MinMaxAlphaModel(locationAttrs_));
        }

        AttributeTable locationAttrs_;
        std::unique_ptr<MinMaxAlphaModel> minMaxModel_;
    };

    // ConstantProbability tests

    TEST(ContactModelFactoryTest, CreatesConstantProbabilityModel)
    {
        AttributeTable attrs = TestHelpers::CreateTestLocationAttributes();

        ContactModel *model = createContactModel(
            static_cast<int>(ContactModelType::constant_probability),
            attrs);

        ASSERT_NE(model, nullptr);

        Location loc = TestHelpers::BuildTestLocation();
        EXPECT_DOUBLE_EQ(model->getContactProbability(loc), 0.5);

        delete model;
    }

    TEST_F(ContactModelTest, DefaultProbabilityIsConstant)
    {
        Location loc = TestHelpers::BuildTestLocation();

        // Default contact probability should be 0.5
        EXPECT_DOUBLE_EQ(contactModel_->getContactProbability(loc), 0.5);
    }

    TEST_F(ContactModelTest, ComputeLocationValuesDoesNotCrash)
    {
        Location loc = TestHelpers::BuildTestLocation();

        // For default ContactModel, this should do nothing
        EXPECT_NO_THROW(contactModel_->computeLocationValues(&loc));
    }

    TEST_F(ContactModelTest, MadeContactStatisticallyCorrect)
    {
        // Statistical test for contact probability
        // Uses 99% confidence interval to reduce flakiness

        Location loc = TestHelpers::BuildTestLocation();

        Event e1 = TestHelpers::BuildTestEvent(0, ARRIVAL, 0);
        Event e2 = TestHelpers::BuildTestEvent(1, ARRIVAL, 0);

        const int trials = 10000;
        int contacts = 0;

        for (int i = 0; i < trials; i++)
        {
            if (contactModel_->madeContact(e1, e2, &loc))
            {
                contacts++;
            }
        }

        double default_probability = 0.5;
        double rate = static_cast<double>(contacts) / trials;

        // Standard error for proportion: sqrt(p*(1-p)/n)
        double std_error = sqrt(default_probability * (1 - default_probability) / trials);
        
        // 99.9% confidence interval (z = 3.29) to minimize flakiness
        const double z_999 = 3.29;
        double margin = z_999 * std_error;

        // Rate should be within [0.5 - margin, 0.5 + margin]
        EXPECT_GE(rate, default_probability - margin) 
            << "Contact rate " << rate << " is too low";
        EXPECT_LE(rate, default_probability + margin)
            << "Contact rate " << rate << " is too high";
    }

    TEST_F(MinMaxAlphaModelTest, EpsilonToEquation)
    {
        const unsigned int MIN = 5;
        const unsigned int MAX = 40;
        const unsigned int ALPHA = 1000;

        const double epsilon = 0.0001;
        const double alpha = static_cast<double>(ALPHA);

        const int numLocations = 20;
        std::vector<Location> locations = TestHelpers::BuildTestLocations(numLocations);

        for (unsigned int index = 0; index < numLocations; ++index)
        {
            Location &location = locations[index];
            std::vector<union Data> &data = location.getData();

            int maxSimVisitsIndex = 1;  // Based on TestHelpers location attributes
            double max_visits =
                static_cast<double>(data[maxSimVisitsIndex].int32_val);

            if (max_visits <= 1.0) {
                continue;  // Skip invalid max_visits values
            }

            double contact_prob_equation =
                (MIN + (MAX - MIN) * (1.0 - exp(-max_visits / alpha))) /
                (max_visits - 1.0);

            minMaxModel_->computeLocationValues(&location);
            double received_contact_prob =
                minMaxModel_->getContactProbability(location);

            EXPECT_NEAR(contact_prob_equation, received_contact_prob, epsilon);
        }
    }

    // idea of minmaxalphamodel (fixed for a given location):
    //   contactProbability.double_val = (MIN + (MAX - MIN) * (1.0 - exp(-max_visits / ALPHA))) / (max_visits - 1));

    // idea of minmaxalphamodel (fixed for a given location):
    // for a minimum number of people at a location (or less) contact probability should be 1
    // for a number of people (maximum simultaneous visits, not current occupancy)
    // greater than the maximum occupancy,
    // probability any pair of people interact == alpha

    // template code to acess MSV:
    //   std::vector<union Data> &data = location.getData();
    //   double max_visits =
    //      static_cast<double>(data[maxSimVisitsIndex].int32_val);

    TEST_F(MinMaxAlphaModelTest, ProbabilityIsValidRange)
    {
        const unsigned int n = 1000;
        for (int i = 0; i < n; i++)
        {
            Location location = TestHelpers::BuildTestLocation(1);

            minMaxModel_->computeLocationValues(&location);

            double prob = minMaxModel_->getContactProbability(location);

            EXPECT_GE(prob, 0.0);
            EXPECT_LE(prob, 1.0);
        }
    }

    TEST(ContactModelFactoryTest, CreatesMinMaxAlphaModel)
    {
        AttributeTable attrs = TestHelpers::CreateTestLocationAttributes();

        ContactModel *model = createContactModel(
            static_cast<int>(ContactModelType::min_max_alpha),
            attrs);

        ASSERT_NE(model, nullptr);

        Location loc = TestHelpers::BuildTestLocation();
        model->computeLocationValues(&loc);

        double prob = model->getContactProbability(loc);
        EXPECT_GE(prob, 0.0);
        EXPECT_LE(prob, 1.0);

        delete model;
    }

} // namespace

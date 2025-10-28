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
            contactModel_ = std::make_unique<ContactModel>(locationAttrs_);
        }

        AttributeTable locationAttrs_;
        std::unique_ptr<ContactModel> contactModel_;
    };

    TEST_F(MinMaxAlphaModelTest, EpsilonToEquation)
    {
        const unsigned int MIN = 5;
        const unsigned int MAX = 40;
        const unsigned int ALPHA = 1000;

        const double epsilon = 0.0001;
        const double alpha = static_cast<double>(ALPHA);

        const int numLocations = 20;
        std::vector<Location> locations = TestHelpers::BuildTestLocations(numLocations);

        for (unsigned int index = 0; index <= numLocations; ++index)
        {
            const Location &location = locations[index % numLocations];
            std::vector<union Data> &data = location->getData();

            double max_visits =
                static_cast<double>(data[maxSimVisitsIndex].int32_val);

            double contact_prob_equation =
                (MIN + (MAX - MIN) * (1.0 - exp(-static_cast<double>(max_visits) / alpha))) /
                (static_cast<double>(max_visits) - 1.0);

            minMaxModel_->computeLocationValues(const_cast<Location *>(&location));
            double received_contact_prob =
                minMaxModel_->getContactProbability(location);

            // double contact_prob_epsilon =
            //     (MIN + (MAX - MIN) * (static_cast<double>(max_visits) / (alpha + static_cast<double>(max_visits)))) /
            //     (static_cast<double>(max_visits) - 1.0 + epsilon);

            EXPECT_NEAR(contact_prob_equation, received_contact_prob, epsilon)

            std::cout << " max_visits: " << max_visits
                      << " equation: " << contact_prob_equation
                      << " epsilon: " << contact_prob_epsilon;
        }
    }

    // also test within epsilon exact equation for contact prob:
    //   contactProbability.double_val = (MIN + (MAX - MIN) * (1.0 - exp(-max_visits / ALPHA))) / (max_visits - 1));

    // idea of minmaxalphamodel (fixed for a given location):
    // for a minimum number of people at a location (or less) contact probability should be 1
    // for a number of people (maximum simultaneous visits, not current occupancy)
    // greater than the maximum occupancy,
    // probability any pair of people interact == alpha

    // template code to acess MSV:
    //   std::vector<union Data> &data = location->getData();
    //   double max_visits =
    //      static_cast<double>(data[maxSimVisitsIndex].int32_val);

    class MinMaxAlphaModelTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            locationAttrs_ = TestHelpers::CreateTestLocationAttributes();
            minMaxModel_ = std::make_unique<MinMaxAlphaModel>(locationAttrs_);
        }

        AttributeTable locationAttrs_;
        std::unique_ptr<MinMaxAlphaModel> minMaxModel_;
    };

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

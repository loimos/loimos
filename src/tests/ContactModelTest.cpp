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


} // namespace

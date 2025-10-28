/* Copyright 2020-2025 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "../Partitioner.h"
#include "../Defs.h"
#include "gtest/gtest.h"

#include <vector>

namespace
{

    // Test the Partitioner class with simple in-memory initialization
    class PartitionerTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            // Create a simple partitioner: 100 people, 10 locations, 4 partitions each
            partitioner_ = std::make_unique<Partitioner>(4, 4, 100, 10);
        }

        std::unique_ptr<Partitioner> partitioner_;
    };

    TEST_F(PartitionerTest, InitializesCorrectly)
    {
        EXPECT_EQ(partitioner_->numPeople, 100);
        EXPECT_EQ(partitioner_->numLocations, 10);
    }

    TEST_F(PartitionerTest, PersonPartitionCount)
    {
        EXPECT_EQ(partitioner_->getNumPersonPartitions(), 4);
    }

    TEST_F(PartitionerTest, LocationPartitionCount)
    {
        EXPECT_EQ(partitioner_->getNumLocationPartitions(), 4);
    }

    TEST_F(PartitionerTest, PersonPartitionSizesAreCorrect)
    {
        // 100 people / 4 partitions = 25 each
        EXPECT_EQ(partitioner_->getPersonPartitionSize(0), 25);
        EXPECT_EQ(partitioner_->getPersonPartitionSize(1), 25);
        EXPECT_EQ(partitioner_->getPersonPartitionSize(2), 25);
        EXPECT_EQ(partitioner_->getPersonPartitionSize(3), 25);
    }

    TEST_F(PartitionerTest, LocationPartitionSizesAreCorrect)
    {
        // 10 locations / 4 partitions = 3, 3, 2, 2
        Id total = 0;
        for (PartitionId p = 0; p < 4; p++)
        {
            Id size = partitioner_->getLocationPartitionSize(p);
            EXPECT_GT(size, 0);
            total += size;
        }
        EXPECT_EQ(total, 10);
    }

    TEST_F(PartitionerTest, PersonPartitionIndexIsCorrect)
    {
        // Person 0-24 should be in partition 0
        EXPECT_EQ(partitioner_->getPersonPartitionIndex(0), 0);
        EXPECT_EQ(partitioner_->getPersonPartitionIndex(24), 0);

        // Person 25-49 should be in partition 1
        EXPECT_EQ(partitioner_->getPersonPartitionIndex(25), 1);
        EXPECT_EQ(partitioner_->getPersonPartitionIndex(49), 1);

        // Person 50-74 should be in partition 2
        EXPECT_EQ(partitioner_->getPersonPartitionIndex(50), 2);
        EXPECT_EQ(partitioner_->getPersonPartitionIndex(74), 2);

        // Person 75-99 should be in partition 3
        EXPECT_EQ(partitioner_->getPersonPartitionIndex(75), 3);
        EXPECT_EQ(partitioner_->getPersonPartitionIndex(99), 3);
    }

    TEST_F(PartitionerTest, LocalGlobalIndexConversionForPeople)
    {
        // Test person 50 (should be in partition 2, local index 0)
        Id globalIdx = 50;
        PartitionId partIdx = partitioner_->getPersonPartitionIndex(globalIdx);
        EXPECT_EQ(partIdx, 2);

        Id localIdx = partitioner_->getLocalPersonIndex(globalIdx, partIdx);
        EXPECT_EQ(localIdx, 0); // First person in partition 2

        // Convert back to global
        Id convertedGlobal = partitioner_->getGlobalPersonIndex(localIdx, partIdx);
        EXPECT_EQ(convertedGlobal, globalIdx);
    }

    TEST_F(PartitionerTest, LocalGlobalIndexConversionForLocations)
    {
        // Test location 5
        Id globalIdx = 5;
        PartitionId partIdx = partitioner_->getLocationPartitionIndex(globalIdx);

        Id localIdx = partitioner_->getLocalLocationIndex(globalIdx, partIdx);

        // Convert back to global
        Id convertedGlobal = partitioner_->getGlobalLocationIndex(localIdx, partIdx);
        EXPECT_EQ(convertedGlobal, globalIdx);
    }

    TEST_F(PartitionerTest, CacheIndexForPeople)
    {
        // Cache index should be the local index for partition 0
        EXPECT_EQ(partitioner_->getPersonCacheIndex(0),
                  partitioner_->getLocalPersonIndex(0, 0));
        EXPECT_EQ(partitioner_->getPersonCacheIndex(10),
                  partitioner_->getLocalPersonIndex(10, 0));
    }

    TEST_F(PartitionerTest, CacheIndexForLocations)
    {
        // Cache index should be the local index for partition 0
        EXPECT_EQ(partitioner_->getLocationCacheIndex(0),
                  partitioner_->getLocalLocationIndex(0, 0));
        EXPECT_EQ(partitioner_->getLocationCacheIndex(5),
                  partitioner_->getLocalLocationIndex(5, 0));
    }

    TEST(PartitionerUnevenTest, HandlesUnevenPartitioning)
    {
        // 103 people across 4 partitions
        // Should be 26, 26, 26, 25
        Partitioner part(4, 1, 103, 1);

        EXPECT_EQ(part.getPersonPartitionSize(0), 26);
        EXPECT_EQ(part.getPersonPartitionSize(1), 26);
        EXPECT_EQ(part.getPersonPartitionSize(2), 26);
        EXPECT_EQ(part.getPersonPartitionSize(3), 25);

        // Verify all people are accounted for
        Id total = 0;
        for (PartitionId p = 0; p < 4; p++)
        {
            total += part.getPersonPartitionSize(p);
        }
        EXPECT_EQ(total, 103);
    }

    TEST(PartitionerUnevenTest, AllPeopleHaveValidPartition)
    {
        // 4 people partitions, 103 people
        // 1 location partition, 1 location
        Partitioner part(4, 1, 103, 1);

        for (Id i = 0; i < 103; i++)
        {
            PartitionId p = part.getPersonPartitionIndex(i);
            EXPECT_GE(p, 0);
            EXPECT_LT(p, 4);
        }
    }

    // Test standalone utility functions
    TEST(PartitionerUtilsTest, GetNumElementsPerPartition)
    {
        // 100 elements / 4 partitions = 25
        EXPECT_EQ(getNumElementsPerPartition(100, 4), 25);

        // 103 elements / 4 partitions = 26 (ceiling)
        EXPECT_EQ(getNumElementsPerPartition(103, 4), 26);
    }

    TEST(PartitionerUtilsTest, GetNumLargerPartitions)
    {
        // 100 elements / 4 partitions = 25 each, so all are "larger"
        EXPECT_EQ(getNumLargerPartitions(100, 4), 4);

        // 103 elements / 4 partitions = 3 have 26, 1 has 25
        EXPECT_EQ(getNumLargerPartitions(103, 4), 3);
    }

    TEST(PartitionerUtilsTest, GetPartitionIndex)
    {
        Id numElements = 100;
        PartitionId numPartitions = 4;
        Id offset = 0;

        // Element 0 should be in partition 0
        EXPECT_EQ(getPartitionIndex(0, numElements, numPartitions, offset), 0);

        // Element 25 should be in partition 1
        EXPECT_EQ(getPartitionIndex(25, numElements, numPartitions, offset), 1);

        // Element 50 should be in partition 2
        EXPECT_EQ(getPartitionIndex(50, numElements, numPartitions, offset), 2);

        // Element 99 should be in partition 3
        EXPECT_EQ(getPartitionIndex(99, numElements, numPartitions, offset), 3);
    }

    TEST(PartitionerUtilsTest, GetFirstIndex)
    {
        Id numElements = 100;
        PartitionId numPartitions = 4;
        Id offset = 0;

        EXPECT_EQ(getFirstIndex(0, numElements, numPartitions, offset), 0);
        EXPECT_EQ(getFirstIndex(1, numElements, numPartitions, offset), 25);
        EXPECT_EQ(getFirstIndex(2, numElements, numPartitions, offset), 50);
        EXPECT_EQ(getFirstIndex(3, numElements, numPartitions, offset), 75);
    }

    TEST(PartitionerUtilsTest, GetNumLocalElements)
    {
        Id numElements = 103;
        PartitionId numPartitions = 4;

        // Should be 26, 26, 26, 25
        EXPECT_EQ(getNumLocalElements(numElements, numPartitions, 0), 26);
        EXPECT_EQ(getNumLocalElements(numElements, numPartitions, 1), 26);
        EXPECT_EQ(getNumLocalElements(numElements, numPartitions, 2), 26);
        EXPECT_EQ(getNumLocalElements(numElements, numPartitions, 3), 25);
    }

    TEST(PartitionerUtilsTest, GetGlobalFromLocal)
    {
        Id numElements = 100;
        PartitionId numPartitions = 4;
        Id offset = 0;

        // Partition 0, local index 0 = global 0
        EXPECT_EQ(getGlobalIndex(0, 0, numElements, numPartitions, offset), 0);

        // Partition 1, local index 0 = global 25
        EXPECT_EQ(getGlobalIndex(0, 1, numElements, numPartitions, offset), 25);

        // Partition 2, local index 10 = global 60
        EXPECT_EQ(getGlobalIndex(10, 2, numElements, numPartitions, offset), 60);
    }

    TEST(PartitionerUtilsTest, GetLocalFromGlobal)
    {
        Id numElements = 100;
        PartitionId numPartitions = 4;
        Id offset = 0;

        // Global 0 in partition 0 = local 0
        EXPECT_EQ(getLocalIndex(0, 0, numElements, numPartitions, offset), 0);

        // Global 25 in partition 1 = local 0
        EXPECT_EQ(getLocalIndex(25, 1, numElements, numPartitions, offset), 0);

        // Global 60 in partition 2 = local 10
        EXPECT_EQ(getLocalIndex(60, 2, numElements, numPartitions, offset), 10);
    }

    TEST(PartitionerUtilsTest, PartitionWithOffsets)
    {
        std::vector<Id> offsets = {0, 25, 50, 75};

        EXPECT_EQ(getPartition(0, offsets), 0);
        EXPECT_EQ(getPartition(24, offsets), 0);
        EXPECT_EQ(getPartition(25, offsets), 1);
        EXPECT_EQ(getPartition(49, offsets), 1);
        EXPECT_EQ(getPartition(50, offsets), 2);
        EXPECT_EQ(getPartition(74, offsets), 2);
        EXPECT_EQ(getPartition(75, offsets), 3);
        EXPECT_EQ(getPartition(99, offsets), 3);
    }

    TEST(PartitionerUtilsTest, GetPartitionSizeFromOffsets)
    {
        std::vector<Id> offsets = {0, 25, 50, 75};
        Id numObjects = 100;

        EXPECT_EQ(getPartitionSize(0, numObjects, offsets), 25);
        EXPECT_EQ(getPartitionSize(1, numObjects, offsets), 25);
        EXPECT_EQ(getPartitionSize(2, numObjects, offsets), 25);
        EXPECT_EQ(getPartitionSize(3, numObjects, offsets), 25);
    }

    TEST(PartitionerUtilsTest, LocalGlobalWithOffsets)
    {
        std::vector<Id> offsets = {0, 30, 60, 90};

        // Global 35 is in partition 1, local index 5
        EXPECT_EQ(getLocalIndex(35, 1, offsets), 5);

        // Partition 1, local 5 = global 35
        EXPECT_EQ(getGlobalIndex(5, 1, offsets), 35);
    }

} // namespace

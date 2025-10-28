/* Copyright 2020-2025 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef TESTS_TESTHELPERS_H_
#define TESTS_TESTHELPERS_H_

#include "../readers/AttributeTable.h"
#include "../Location.h"
#include "../Person.h"
#include "../Event.h"

namespace TestHelpers
{

    // Create a minimal person attribute table for testing
    inline AttributeTable CreateTestPersonAttributes()
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

    // Create a minimal location attribute table for testing
    inline AttributeTable CreateTestLocationAttributes()
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

    // Build a test location with minimal setup
    inline Location BuildTestLocation(int uniqueId = 0, int numInterventions = 0, int numDays = 1)
    {
        AttributeTable attrs = CreateTestLocationAttributes();
        Location loc(attrs, numInterventions, uniqueId, numDays);
        loc.setSeed(12345);
        return loc;
    }

    // Builds a test location with minimal setup; uniqueId is set from 0 to numLocations-1, seeds are set to 12345 + uniqueId
    std::vector<Location> BuildTestLocations(int numLocations, int numInterventions = 0, int numDays = 1)
    {
        AttributeTable attrs = CreateTestLocationAttributes();
        std::vector<Location> locations;
        for (int i = 0; i < numLocations; i++)
        {
            Location loc(attrs, numInterventions, i, numDays);
            loc.setSeed(12345 + i);
            locations.push_back(loc);
        }
        return locations;
    }

    // Build a test event
    inline Event BuildTestEvent(Id personIdx, EventType type, Time scheduledTime,
                                DiseaseState personState = 0, double transmissionModifier = 1.0)
    {
        Event e;
        e.personIdx = personIdx;
        e.type = type;
        e.scheduledTime = scheduledTime;
        e.personState = personState;
        e.transmissionModifier = transmissionModifier;
        e.partnerTime = 0;
        return e;
    }

} // namespace TestHelpers

#endif // TESTS_TESTHELPERS_H_

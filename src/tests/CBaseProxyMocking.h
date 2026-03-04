#pragma once

#define MOCK_CBASE(type) struct CBase_##type {};

#define ALL_CBASE_TYPES \
    X(Scenario)

#define X(type) MOCK_CBASE(type)
ALL_CBASE_TYPES
#undef X

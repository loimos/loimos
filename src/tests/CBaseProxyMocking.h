#pragma once

#define MOCK_CBASE(type) struct CBase_##type {};

#define ALL_CBASE_TYPES \
    X(Scenario)

#define X(type) MOCK_CBASE(type)
ALL_CBASE_TYPES
#undef X

// Macro to mock proxies
#define MOCK_PROXY(type) struct CProxy_##type {};

// List all proxies you need
#define ALL_PROXIES \
    X(Main)        \
    X(People)      \
    X(Locations)   \
    X(Scenario)    \
    X(Aggregator)  // include only if USE_HYPERCOMM

// Expand the list
#define X(type) MOCK_PROXY(type)
ALL_PROXIES
#undef X
#undef MOCK_PROXY

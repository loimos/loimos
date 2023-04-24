#ifndef __TYPES_H__
#define __TYPES_H__

// Event types
using EventType = char;
using Time = int32_t;

// For counting events (interactions, visits, exposures...)
using Counter = uint64_t;
#define COUNTER_PRINT_TYPE "%lu"
#define COUNTER_REDUCTION_TYPE ulong

#endif

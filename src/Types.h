/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef TYPES_H_
#define TYPES_H_

#include <cstdint>
#include <string>

// For object I/O cache
using CacheOffset = uint64_t;

// For locating people or locations
using Id = int32_t;
#define ID_PRINT_TYPE "%d"
#define ID_PROTOBUF_TYPE int_b10
#define ID_PARSE std::stoi

// Disease states
using DiseaseState = int16_t;

// Event types
using EventType = char;
using Time = int32_t;
#define TIME_PRINT_TYPE "%d"
//#define TIME_PROTOBUF_TYPE int_b10
#define TIME_PARSE std::atoi

// For counting events (interactions, visits, exposures...)
using Counter = double;
#define COUNTER_PRINT_TYPE "%0.0f"
#define COUNTER_REDUCTION_TYPE double

#endif  // TYPES_H_

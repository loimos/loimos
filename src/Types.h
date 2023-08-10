/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef TYPES_H_
#define TYPES_H_

#include <cstdint>

// Event types
using EventType = char;
using Time = int32_t;

// For counting events (interactions, visits, exposures...)
using Counter = double;
#define COUNTER_PRINT_TYPE "%0.0f"
#define COUNTER_REDUCTION_TYPE double

#endif  // TYPES_H_

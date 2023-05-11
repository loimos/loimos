/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef TYPES_H_
#define TYPES_H_

// Event types
using EventType = char;
using Time = int32_t;

// For counting events (interactions, visits, exposures...)
using Counter = uint64_t;
#define COUNTER_PRINT_TYPE "%lu"
#define COUNTER_REDUCTION_TYPE ulong

#endif  // TYPES_H_

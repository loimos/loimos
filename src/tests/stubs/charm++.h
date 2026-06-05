/* Copyright 2026 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 *
 * Minimal Charm++ stubs for unit testing without Charm++.
 */
#ifndef TESTS_STUBS_CHARM_PP_H_
#define TESTS_STUBS_CHARM_PP_H_

#include "pup.h"

#include <cassert>

struct CkMigrateMessage {};
struct CkEntryOptions {};

extern "C" {
void CkAbort(const char *format, ...);
void CmiAbort(const char *format, ...);
void CkPrintf(const char *format, ...);
void CmiPrintf(const char *format, ...);
int CkMyPe();
int CkNumPes();
int CmiMyPe();
int CmiNumPes();
}

static inline int CkMyNode() {
  return 0;
}

static inline int CkNodeSize(int) {
  return 1;
}

#define CkAssert(expr) assert(expr)

#endif  // TESTS_STUBS_CHARM_PP_H_

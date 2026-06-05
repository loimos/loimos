/* Copyright 2020-2025 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 *
 * Stub implementations of Charm++ functions for unit testing.
 * This allows unit tests to compile and link without the full Charm++ runtime.
 */

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <string>
#include "gtest/gtest.h"

// Helper: format a va_list message into a std::string.
static std::string FormatVA(const char* format, va_list args) {
    // Measure required size.
    va_list args_copy;
    va_copy(args_copy, args);
    int n = vsnprintf(nullptr, 0, format, args_copy);
    va_end(args_copy);
    if (n < 0) return "(vsnprintf error)";
    std::string buf(static_cast<size_t>(n) + 1, '\0');
    vsnprintf(&buf[0], buf.size(), format, args);
    buf.resize(static_cast<size_t>(n));
    return buf;
}

extern "C" {

// Global variables used by Charm++
int _Cmi_mype = 0;      // Current PE (processor element)
int _Cmi_numpes = 1;    // Total number of PEs

// CkAbort - log a non-fatal GoogleTest failure so the test is marked FAILED
// but the process keeps running (no abort()).
void CkAbort(const char* format, ...) {
    va_list args;
    va_start(args, format);
    std::string msg = FormatVA(format, args);
    va_end(args);
    GTEST_FAIL() << "CkAbort: " << msg;
}

// CmiAbort - same treatment as CkAbort for unit-test builds.
void CmiAbort(const char* format, ...) {
    va_list args;
    va_start(args, format);
    std::string msg = FormatVA(format, args);
    va_end(args);
    GTEST_FAIL() << "CmiAbort: " << msg;
}

// CkPrintf - Charm++ printf
void CkPrintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

// CmiPrintf - lower-level printf
void CmiPrintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

// CkMyPe - return current PE (processor element) - always 0 for unit tests
int CkMyPe() {
    return 0;
}

// CkNumPes - return number of PEs - always 1 for unit tests
int CkNumPes() {
    return 1;
}

// CmiMyPe - lower-level version
int CmiMyPe() {
    return 0;
}

// CmiNumPes - lower-level version
int CmiNumPes() {
    return 1;
}

}  // extern "C"

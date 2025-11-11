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

extern "C" {

// Global variables used by Charm++
int _Cmi_mype = 0;      // Current PE (processor element)
int _Cmi_numpes = 1;    // Total number of PEs

// CkAbort - called for fatal errors
void CkAbort(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "CkAbort: ");
    vfprintf(stderr, format, args);
    va_end(args);
    abort();
}

// CmiAbort - lower-level abort
void CmiAbort(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "CmiAbort: ");
    vfprintf(stderr, format, args);
    va_end(args);
    abort();
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

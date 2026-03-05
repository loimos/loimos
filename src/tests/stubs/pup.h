/* Copyright 2026 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 *
 * Minimal PUP stubs for unit testing without Charm++.
 */
#ifndef TESTS_STUBS_PUP_H_
#define TESTS_STUBS_PUP_H_

namespace PUP {
class er {
 public:
  // State query methods used by production pup() implementations
  bool isPacking() const { return false; }
  bool isUnpacking() const { return false; }
  bool isSizing() const { return false; }
  bool isDeleting() const { return false; }

  template <typename T>
  er &operator|(T &) {
    return *this;
  }
  template <typename T>
  er &operator|(const T &) {
    return *this;
  }
};
}  // namespace PUP

#ifndef PUPbytes
#define PUPbytes(type) static_assert(sizeof(type) >= 0, "PUPbytes stub requires complete type")
#endif

#endif  // TESTS_STUBS_PUP_H_

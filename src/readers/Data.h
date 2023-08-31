/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef READERS_DATA_H_
#define READERS_DATA_H_

#include "charm++.h"

#include <string>

namespace DataTypes {
  enum DataType {int32_, int64_, uint32_, uint64_, string_, double_, category_, bool_};
}

union Data {
  int32_t int32_val;
  int64_t int64_val;
  bool bool_val;
  uint32_t uint32_val;
  uint64_t uint64_val;
  double double_val;
  uint16_t category_val;
  std::string *string_val;
};
PUPbytes(union Data);

#endif  // READERS_DATA_H_

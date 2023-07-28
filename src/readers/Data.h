/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef DATA_H_
#define DATA_H_

#include "charm++.h"

namespace DataTypes {
  enum DataType {int_b10, uint_32, string, double_b10, category, boolean};
}

union Data {
  int int_b10;
  bool boolean;
  uint32_t uint_32;
  double double_b10;
  uint16_t category;
  std::string *str;
};
PUPbytes(union Data);

#endif  // DATA_H_
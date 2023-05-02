/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AGGREGATORPARAM_H_
#define AGGREGATORPARAM_H_

struct AggregatorParam {
  bool useAggregator;
  size_t bufferSize;
  double threshold;
  double flushPeriod;
  bool nodeLevel;

  AggregatorParam() :
    useAggregator(false), bufferSize(0), threshold(0), flushPeriod(0),
      nodeLevel(false) {}

  AggregatorParam(bool useAggregator_, size_t bufferSize_,
      double threshold_, double flushPeriod_, bool nodeLevel_) :
    useAggregator(useAggregator_), bufferSize(bufferSize_),
    threshold(threshold_), flushPeriod(flushPeriod_),
    nodeLevel(nodeLevel_) {}
};
PUPbytes(AggregatorParam);

#endif  // AGGREGATORPARAM_H_
